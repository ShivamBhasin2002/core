/*
 *	Loader Library by Parra Studios
 *	A plugin for loading extension code at run-time into a process.
 *
 *	Copyright (C) 2016 - 2022 Vicente Eduardo Ferrer Garcia <vic798@gmail.com>
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 */

#include <ext_loader/ext_loader_impl.h>

#include <loader/loader.h>
#include <loader/loader_impl.h>

#include <portability/portability_path.h>

#include <reflect/reflect_context.h>
#include <reflect/reflect_function.h>
#include <reflect/reflect_scope.h>
#include <reflect/reflect_type.h>

#include <log/log.h>

#include <filesystem>
#include <set>
#include <string>
#include <vector>

typedef struct loader_impl_ext_type
{
	std::set<std::filesystem::path> paths;

} * loader_impl_ext;

typedef struct loader_impl_ext_handle_lib_type
{
	std::string name;
	dynlink handle;
	dynlink_symbol_addr addr;

} * loader_impl_ext_handle_lib;

typedef struct loader_impl_ext_handle_type
{
	std::vector<loader_impl_ext_handle_lib_type> extensions;

} * loader_impl_ext_handle;

dynlink ext_loader_impl_load_from_file_dynlink(loader_impl_ext ext_impl, const loader_path path);
int ext_loader_impl_load_from_file_handle(loader_impl_ext ext_impl, loader_impl_ext_handle ext_handle, const loader_path path);
static void ext_loader_impl_destroy_handle(loader_impl_ext_handle ext_handle);

int function_ext_interface_create(function func, function_impl impl)
{
	(void)func;
	(void)impl;

	return 0;
}

function_return function_ext_interface_invoke(function func, function_impl impl, function_args args, size_t size)
{
	/* TODO */

	(void)func;
	(void)impl;
	(void)args;
	(void)size;

	return NULL;
}

function_return function_ext_interface_await(function func, function_impl impl, function_args args, size_t size, function_resolve_callback resolve_callback, function_reject_callback reject_callback, void *context)
{
	/* TODO */

	(void)func;
	(void)impl;
	(void)args;
	(void)size;
	(void)resolve_callback;
	(void)reject_callback;
	(void)context;

	return NULL;
}

void function_ext_interface_destroy(function func, function_impl impl)
{
	/* TODO */

	(void)func;
	(void)impl;
}

function_interface function_ext_singleton(void)
{
	static struct function_interface_type ext_interface = {
		&function_ext_interface_create,
		&function_ext_interface_invoke,
		&function_ext_interface_await,
		&function_ext_interface_destroy
	};

	return &ext_interface;
}

int ext_loader_impl_initialize_types(loader_impl impl)
{
	for (type_id index = 0; index < TYPE_SIZE; ++index)
	{
		type t = type_create(index, type_id_name(index), NULL, NULL);

		if (t != NULL)
		{
			if (loader_impl_type_define(impl, type_name(t), t) != 0)
			{
				type_destroy(t);
				return 1;
			}
		}
	}

	return 0;
}

loader_impl_data ext_loader_impl_initialize(loader_impl impl, configuration config)
{
	loader_impl_ext ext_impl = new loader_impl_ext_type();

	(void)impl;
	(void)config;

	if (ext_impl == nullptr)
	{
		return NULL;
	}

	if (ext_loader_impl_initialize_types(impl) != 0)
	{
		delete ext_impl;

		return NULL;
	}

	/* Register initialization */
	loader_initialization_register(impl);

	return static_cast<loader_impl_data>(ext_impl);
}

int ext_loader_impl_execution_path(loader_impl impl, const loader_path path)
{
	loader_impl_ext ext_impl = static_cast<loader_impl_ext>(loader_impl_get(impl));

	ext_impl->paths.insert(std::filesystem::path(path));

	return 0;
}

dynlink ext_loader_impl_load_from_file_dynlink(loader_impl_ext ext_impl, const loader_path path)
{
	for (auto exec_path : ext_impl->paths)
	{
		dynlink lib = dynlink_load(exec_path.string().c_str(), path, DYNLINK_FLAGS_BIND_LAZY | DYNLINK_FLAGS_BIND_GLOBAL);

		if (lib != NULL)
		{
			return lib;
		}
	}

	return NULL;
}

int ext_loader_impl_load_from_file_handle(loader_impl_ext ext_impl, loader_impl_ext_handle ext_handle, const loader_path path)
{
	dynlink lib = ext_loader_impl_load_from_file_dynlink(ext_impl, path);

	if (lib == NULL)
	{
		ext_loader_impl_destroy_handle(ext_handle);

		log_write("metacall", LOG_LEVEL_ERROR, "Failed to load extension: %s", path);

		return 1;
	}

	dynlink_symbol_addr symbol_address = NULL;

	if (dynlink_symbol(lib, path, &symbol_address) != 0)
	{
		ext_loader_impl_destroy_handle(ext_handle);

		log_write("metacall", LOG_LEVEL_ERROR, "Failed to load symbol from extension: %s", path);

		return 1;
	}

	loader_impl_ext_handle_lib_type ext_handle_lib = { path, lib, symbol_address };

	ext_handle->extensions.push_back(ext_handle_lib);

	return 0;
}

loader_handle ext_loader_impl_load_from_file(loader_impl impl, const loader_path paths[], size_t size)
{
	loader_impl_ext ext_impl = static_cast<loader_impl_ext>(loader_impl_get(impl));

	if (ext_impl == NULL)
	{
		return NULL;
	}

	loader_impl_ext_handle ext_handle = new loader_impl_ext_handle_type();

	if (ext_handle == nullptr)
	{
		return NULL;
	}

	for (size_t iterator = 0; iterator < size; ++iterator)
	{
		if (ext_loader_impl_load_from_file_handle(ext_impl, ext_handle, paths[iterator]) != 0)
		{
			return NULL;
		}
	}

	return ext_handle;
}

loader_handle ext_loader_impl_load_from_memory(loader_impl impl, const loader_name name, const char *buffer, size_t size)
{
	(void)impl;
	(void)name;
	(void)buffer;
	(void)size;

	/* TODO: Here we should load the symbols from the process itself */

	return NULL;
}

loader_handle ext_loader_impl_load_from_package(loader_impl impl, const loader_path path)
{
	loader_impl_ext ext_impl = static_cast<loader_impl_ext>(loader_impl_get(impl));

	if (ext_impl == NULL)
	{
		return NULL;
	}

	loader_impl_ext_handle ext_handle = new loader_impl_ext_handle_type();

	if (ext_handle == nullptr)
	{
		return NULL;
	}

	if (ext_loader_impl_load_from_file_handle(ext_impl, ext_handle, path) != 0)
	{
		return NULL;
	}

	return ext_handle;
}

void ext_loader_impl_destroy_handle(loader_impl_ext_handle ext_handle)
{
	for (auto ext : ext_handle->extensions)
	{
		if (ext.handle != NULL)
		{
			dynlink_unload(ext.handle);
		}
	}

	delete ext_handle;
}

int ext_loader_impl_clear(loader_impl impl, loader_handle handle)
{
	loader_impl_ext_handle ext_handle = static_cast<loader_impl_ext_handle>(handle);

	(void)impl;

	if (ext_handle != NULL)
	{
		ext_loader_impl_destroy_handle(ext_handle);

		return 0;
	}

	return 1;
}

int ext_loader_impl_discover(loader_impl impl, loader_handle handle, context ctx)
{
	/* Here we should invoke the entry point function and retrieve the context data */
	(void)impl;
	(void)handle;
	(void)ctx;

	return 0;
}

int ext_loader_impl_destroy(loader_impl impl)
{
	loader_impl_ext ext_impl = static_cast<loader_impl_ext>(loader_impl_get(impl));

	if (ext_impl != NULL)
	{
		/* Destroy children loaders */
		loader_unload_children(impl);

		delete ext_impl;

		return 0;
	}

	return 1;
}
