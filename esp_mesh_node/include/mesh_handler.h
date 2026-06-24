#pragma once
#include "app_config.h"
#include <painlessMesh.h>

extern painlessMesh mesh;

void mesh_init();
void mesh_task();
void mesh_stop();
bool mesh_is_started();
void meshInfo();