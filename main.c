#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <dlfcn.h>

#define ERR(...) { \
	fprintf(stderr, __VA_ARGS__); \
	fflush(stderr); \
}

#define OUT(...) { \
	fprintf(stdout, __VA_ARGS__); \
	fflush(stdout); \
}

#define STR(s) #s

struct gdk_touch_event{
	int type;
	void *window;
	uint8_t send_event;
	uint32_t time;
	double x;
	double y;
	double *axes;
	uint state;
	void *sequence;
	int emulating_pointer;
	void *device;
	double x_root, y_root;
};

long (*g_signal_connect_data_real)(void *instance, char *detailed_signal, void *c_handler, void *data, void *destroy_data, int connect_flags) = NULL;

#define GDK_TOUCH_BEGIN 37
#define GDK_TOUCH_UPDATE 38
#define GDK_TOUCH_END 39
#define GTK_TOUCH_CANCEL 40

struct id_mapper_entry{
	bool in_use;
	uint64_t id;
};

int (*gd_touch_event_orig)(void *widget, struct gdk_touch_event *touch_event, void *data) = NULL;
static int gd_touch_event_wrapped(void *widget, struct gdk_touch_event *touch_event, void *data){
	static struct id_mapper_entry id_mapper[10] = {0};

	uint64_t sequence = (uint64_t)touch_event->sequence;

	switch(touch_event->type){
		case GDK_TOUCH_BEGIN:{
			for (int i = 0; i < sizeof(id_mapper) / sizeof(struct id_mapper_entry);i++){
				if (!id_mapper[i].in_use){
					id_mapper[i].in_use = true;
					id_mapper[i].id = sequence;
					touch_event->sequence = (void *)i;
					break;
				}
			}
			break;
		}
		case GDK_TOUCH_UPDATE:{
			for (int i = 0; i < sizeof(id_mapper) / sizeof(struct id_mapper_entry);i++){
				if (id_mapper[i].in_use && id_mapper[i].id == sequence){
					touch_event->sequence = (void *)i;
					break;
				}
			}
			break;
		}
		case GDK_TOUCH_END:
		case GTK_TOUCH_CANCEL:{
			for (int i = 0; i < sizeof(id_mapper) / sizeof(struct id_mapper_entry);i++){
				if (id_mapper[i].in_use && id_mapper[i].id == sequence){
					id_mapper[i].in_use = false;
					touch_event->sequence = (void *)i;
					break;
				}
			}
			break;
		}
	}

	return gd_touch_event_orig(widget, touch_event, data);
}

long g_signal_connect_data(void *instance, char *detailed_signal, void *c_handler, void *data, void *destroy_data, int connect_flags){
	//ERR("%s: %s\n", __func__, detailed_signal);

	if (strcmp(detailed_signal, "touch-event") != 0){
		return g_signal_connect_data_real(instance, detailed_signal, c_handler, data, destroy_data, connect_flags);
	}

	gd_touch_event_orig = c_handler;
	ERR("qemu_xwayland_gtk_multitouch_patch: gd_touch_event_orig registered as %p\n", c_handler);
	return g_signal_connect_data_real(instance, detailed_signal, gd_touch_event_wrapped, data, destroy_data, connect_flags);
}

static char *paths[] = {
	"/usr/lib64/libgtk-3.so.0",
	"/usr/lib/libgtk-3.so.0",
	"/usr/lib32/libgtk-3.so.0"
};

static void load_functions(){
	void *libgtk3 = NULL;
	for (int i = 0;i < sizeof(paths) / sizeof(char *);i++){
		libgtk3 = dlopen(paths[i], RTLD_NOW);
		if (libgtk3 != NULL){
			OUT("libgtk-3 opened at %s\n", paths[i]);
			break;
		}
		ERR("cannot open %s, %s\n", paths[i], dlerror());
	}
	if (libgtk3 == NULL){
		ERR("failed opening libgtk-3\n");
		exit(1);
	}
	#define FETCH_FUNCTION(lib, name) { \
		name##_real = dlsym(lib, STR(name)); \
		if (name##_real == NULL){ \
			ERR("failed fetching " STR(name)); \
			exit(1); \
		} \
	}

	FETCH_FUNCTION(libgtk3, g_signal_connect_data);
}

__attribute__((constructor))
static void init(){
	load_functions();
}
