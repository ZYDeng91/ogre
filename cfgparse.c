#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <libxml/parser.h>

#include "cfgparse.h"

#define DEFAULT_STREAM_NAME "unnamed ices stream"
#define DEFAULT_STREAM_GENRE "ices unset"
#define DEFAULT_STREAM_DESCRIPTION "no description set"
#define DEFAULT_PLAYLIST_MODULE "playlist"
#define DEFAULT_HOSTNAME "localhost"
#define DEFAULT_PORT 8000
#define DEFAULT_PASSWORD "password"
#define DEFAULT_USERNAME NULL
#define DEFAULT_MOUNT "/stream.ogg"

/* helper macros so we don't have to write the same
** stupid code over and over
*/
#define SET_STRING(x) \
    do {\
        if (x) xmlFree(x);\
        (x) = (char *)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);\
        if (!(x)) (x) = xmlStrdup("");\
    } while (0) 

#define SET_INT(x) \
    do {\
        char *tmp = (char *)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);\
        (x) = atoi(tmp);\
        if (tmp) xmlFree(tmp);\
    } while (0)

#define SET_PARM_STRING(p,x) \
        do {\
                if (x) xmlFree(x);\
                (x) = (char *)xmlGetProp(node, p);\
    } while (0)

/* this is the global config variable */
config_t *config_glob;

static int _using_default_instance = 1;

static void _free_instances(instance_t *instance)
{
    instance_t *next;

    next = NULL;
    do 
    {
        config_free_instance(instance);
        next = instance->next;
        free(instance);

        instance = next;
    } while (next != NULL);
}

void config_free_instance(instance_t *instance)
{
    if (instance->hostname) xmlFree(instance->hostname);
    if (instance->password) xmlFree(instance->password);
    if (instance->user) xmlFree(instance->user);
    if (instance->mount) xmlFree(instance->mount);
}

static void _set_instance_defaults(instance_t *instance)
{
    instance->hostname = xmlStrdup(DEFAULT_HOSTNAME);
    instance->port = DEFAULT_PORT;
    instance->password = xmlStrdup(DEFAULT_PASSWORD);
    instance->user = DEFAULT_USERNAME;
    instance->mount = xmlStrdup(DEFAULT_MOUNT);

    instance->next = NULL;
}

static void _parse_metadata(config_t *config, 
        xmlDocPtr doc, xmlNodePtr node)
{
    do 
    {
        if (node == NULL) break;
        if (xmlIsBlankNode(node)) continue;

        if (strcmp(node->name, "name") == 0) {
            SET_STRING(config->stream_name);
        }
        else if (strcmp(node->name, "genre") == 0) {
            SET_STRING(config->stream_genre);
        }
        else if (strcmp(node->name, "description") == 0) {
            SET_STRING(config->stream_description);
        }
	else if (strcmp(node->name, "url") == 0) {
	    SET_STRING(config->stream_url);
	}
    } while ((node = node->next));
}

static void _parse_instance(config_t *config, xmlDocPtr doc, xmlNodePtr node)
{
    instance_t *instance, *i;

    instance = (instance_t *)calloc(1, sizeof(instance_t));
    _set_instance_defaults(instance);

    do 
    {
        if (node == NULL) break;
        if (xmlIsBlankNode(node)) continue;

        if (strcmp(node->name, "hostname") == 0)
            SET_STRING(instance->hostname);
        else if (strcmp(node->name, "port") == 0)
            SET_INT(instance->port);
        else if (strcmp(node->name, "password") == 0)
            SET_STRING(instance->password);
        else if (strcmp(node->name, "username") == 0)
            SET_STRING(instance->user);
        else if (strcmp(node->name, "mount") == 0)
            SET_STRING(instance->mount);
    } while ((node = node->next));

    instance->next = NULL;

    if (_using_default_instance) 
    {
        _using_default_instance = 0;
        _free_instances(config->instances);
        config->instances = NULL;
    }

    if (config->instances == NULL) 
    {
        config->instances = instance;
    } 
    else 
    {
        i = config->instances;
        while (i->next != NULL) i = i->next;
        i->next = instance;
    }
}

static void _parse_input(config_t *config, xmlDocPtr doc, xmlNodePtr node)
{
    module_param_t *param, *p;

    do 
    {
        if (node == NULL) break;
        if (xmlIsBlankNode(node)) continue;

        if (strcmp(node->name, "module") == 0)
            SET_STRING(config->playlist_module);
        else if (strcmp(node->name, "param") == 0) {
            param = (module_param_t *)calloc(1, sizeof(module_param_t));
            SET_PARM_STRING("name", param->name);
            SET_STRING(param->value);
            param->next = NULL;

            if (config->module_params == NULL) 
            {
                config->module_params = param;
            } 
            else 
            {
                p = config->module_params;
                while (p->next != NULL) p = p->next;
                p->next = param;
            }
        }
    } while ((node = node->next));
}

static void _parse_stream(config_t *config, xmlDocPtr doc, xmlNodePtr node)
{
    do 
    {
        if (node == NULL) break;
        if (xmlIsBlankNode(node)) continue;

        if (strcmp(node->name, "metadata") == 0)
            _parse_metadata(config, doc, node->xmlChildrenNode);
        else if (strcmp(node->name, "playlist") == 0)
            _parse_input(config, doc, node->xmlChildrenNode);
        else if (strcmp(node->name, "instance") == 0)
            _parse_instance(config, doc, node->xmlChildrenNode);
    } while ((node = node->next));
}

static void _parse_root(config_t *config, xmlDocPtr doc, xmlNodePtr node)
{
    do 
    {
        if (node == NULL) break;
        if (xmlIsBlankNode(node)) continue;

        if (strcmp(node->name, "stream") == 0)
            _parse_stream(config, doc, node->xmlChildrenNode);
    } while ((node = node->next));
}

static void _set_defaults(config_t *c)
{
    instance_t *instance;

    c->stream_name = xmlStrdup(DEFAULT_STREAM_NAME);
    c->stream_genre = xmlStrdup(DEFAULT_STREAM_GENRE);
    c->stream_description = xmlStrdup(DEFAULT_STREAM_DESCRIPTION);
    c->stream_url = NULL;

    c->playlist_module = xmlStrdup(DEFAULT_PLAYLIST_MODULE);

    c->module_params = NULL;

    instance = (instance_t *)malloc(sizeof(instance_t));
    _set_instance_defaults(instance);
    c->instances = instance;
}

static void _free_params(module_param_t *param)
{
    module_param_t *next;
    next = NULL;
    do 
    {
        if (param->name) free(param->name);
        if (param->value) free(param->value);
        next = param->next;
        free(param);

        param = next;
    } while (next != NULL);
}

void config_init(void)
{
    config_glob = (config_t *)calloc(1, sizeof(config_t));
    xmlInitParser();
    _set_defaults(config_glob);
    srandom(time(NULL));
}

void config_shutdown(void)
{
    if (config_glob == NULL) return;

    if (config_glob->module_params != NULL) 
    {
        _free_params(config_glob->module_params);
        config_glob->module_params = NULL;
    }

    if (config_glob->instances != NULL) 
    {
        _free_instances(config_glob->instances);
        config_glob->instances = NULL;
    }

    if (config_glob->stream_name)
       xmlFree(config_glob->stream_name);
    if (config_glob->stream_genre)
       xmlFree(config_glob->stream_genre);
    if (config_glob->stream_description)
       xmlFree(config_glob->stream_description);
    if (config_glob->stream_url)
       xmlFree(config_glob->stream_url);

    free(config_glob);
    config_glob = NULL;
    xmlCleanupParser();
}

int config_read(const char *fn)
{
    xmlDocPtr doc;
    xmlNodePtr node;

    if (fn == NULL || strcmp(fn, "") == 0) return -1;

    doc = xmlParseFile(fn);
    if (doc == NULL) return -1;

    node = xmlDocGetRootElement(doc);
    if (node == NULL || strcmp(node->name, "config") != 0) 
    {
        xmlFreeDoc(doc);
        return 0;
    }

    _parse_root(config_glob, doc, node->xmlChildrenNode);

    xmlFreeDoc(doc);

    return 1;
}
