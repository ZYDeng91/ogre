#ifndef __CONFIG_H__
#define __CONFIG_H__

typedef struct _module_param_tag
{
    char *name;
    char *value;

    struct _module_param_tag *next;
} module_param_t;

typedef struct _instance_tag
{
    char *hostname;
    int port;
    char *password;
    char *mount;
    char *user;

    struct _instance_tag *next;
} instance_t;

typedef struct _config_tag
{
    /* <metadata> */

    char *stream_name;
    char *stream_genre;
    char *stream_description;
    char *stream_url;

    /* <playlist> */

    char *playlist_module;
    module_param_t *module_params;

    /* <instance> */

    instance_t *instances;
    
    /* private */
    struct _config_tag *next;
} config_t;

extern config_t *config_glob;

void config_init(void);
void config_shutdown(void);

int config_read(const char *filename);

void config_free_instance(instance_t *instance);

#endif /* __CONFIG_H__ */
