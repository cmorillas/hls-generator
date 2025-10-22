// cef_function_wrappers.cpp
// Auto-generated wrapper functions that call our dynamically loaded function pointers
// These provide the actual symbols that libcef_dll_wrapper expects to link against

#include "cef_loader.h"

extern "C" {

int cef_add_cross_origin_whitelist_entry(const cef_string_t* source_origin,
    const cef_string_t* target_protocol,
    const cef_string_t* target_domain,
    int allow_target_subdomains) {
    if (CEFLib::cef_add_cross_origin_whitelist_entry)
        return CEFLib::cef_add_cross_origin_whitelist_entry(source_origin, target_protocol, target_domain, allow_target_subdomains);
    return 0;
}

struct _cef_binary_value_t* cef_base64decode(const cef_string_t* data) {
    if (CEFLib::cef_base64decode)
        return CEFLib::cef_base64decode(data);
    return nullptr;
}

cef_string_userfree_t cef_base64encode(const void* data,
                                                  size_t data_size) {
    if (CEFLib::cef_base64encode)
        return CEFLib::cef_base64encode(data, data_size);
    return (cef_string_userfree_t){0};
}

cef_basetime_t cef_basetime_now(void) {
    if (CEFLib::cef_basetime_now)
        return CEFLib::cef_basetime_now();
    return (cef_basetime_t){0};
}

int cef_begin_tracing(const cef_string_t* categories,
                                 struct _cef_completion_callback_t* callback) {
    if (CEFLib::cef_begin_tracing)
        return CEFLib::cef_begin_tracing(categories, callback);
    return 0;
}

cef_binary_value_t* cef_binary_value_create(const void* data,
                                                       size_t data_size) {
    if (CEFLib::cef_binary_value_create)
        return CEFLib::cef_binary_value_create(data, data_size);
    return nullptr;
}

int cef_browser_host_create_browser(const cef_window_info_t* windowInfo,
    struct _cef_client_t* client,
    const cef_string_t* url,
    const struct _cef_browser_settings_t* settings,
    struct _cef_dictionary_value_t* extra_info,
    struct _cef_request_context_t* request_context) {
    if (CEFLib::cef_browser_host_create_browser)
        return CEFLib::cef_browser_host_create_browser(windowInfo, client, url, settings, extra_info, request_context);
    return 0;
}

cef_browser_t* cef_browser_host_create_browser_sync(const cef_window_info_t* windowInfo,
    struct _cef_client_t* client,
    const cef_string_t* url,
    const struct _cef_browser_settings_t* settings,
    struct _cef_dictionary_value_t* extra_info,
    struct _cef_request_context_t* request_context) {
    if (CEFLib::cef_browser_host_create_browser_sync)
        return CEFLib::cef_browser_host_create_browser_sync(windowInfo, client, url, settings, extra_info, request_context);
    return nullptr;
}

int cef_clear_cross_origin_whitelist(void) {
    if (CEFLib::cef_clear_cross_origin_whitelist)
        return CEFLib::cef_clear_cross_origin_whitelist();
    return 0;
}

int cef_clear_scheme_handler_factories(void) {
    if (CEFLib::cef_clear_scheme_handler_factories)
        return CEFLib::cef_clear_scheme_handler_factories();
    return 0;
}

cef_command_line_t* cef_command_line_create(void) {
    if (CEFLib::cef_command_line_create)
        return CEFLib::cef_command_line_create();
    return nullptr;
}

cef_command_line_t* cef_command_line_get_global(void) {
    if (CEFLib::cef_command_line_get_global)
        return CEFLib::cef_command_line_get_global();
    return nullptr;
}

cef_cookie_manager_t* cef_cookie_manager_get_global_manager(struct _cef_completion_callback_t* callback) {
    if (CEFLib::cef_cookie_manager_get_global_manager)
        return CEFLib::cef_cookie_manager_get_global_manager(callback);
    return nullptr;
}

int cef_crash_reporting_enabled(void) {
    if (CEFLib::cef_crash_reporting_enabled)
        return CEFLib::cef_crash_reporting_enabled();
    return 0;
}

cef_request_context_t* cef_create_context_shared(cef_request_context_t* other,
    struct _cef_request_context_handler_t* handler) {
    if (CEFLib::cef_create_context_shared)
        return CEFLib::cef_create_context_shared(other, handler);
    return nullptr;
}

int cef_create_directory(const cef_string_t* full_path) {
    if (CEFLib::cef_create_directory)
        return CEFLib::cef_create_directory(full_path);
    return 0;
}

int cef_create_new_temp_directory(const cef_string_t* prefix,
                                             cef_string_t* new_temp_path) {
    if (CEFLib::cef_create_new_temp_directory)
        return CEFLib::cef_create_new_temp_directory(prefix, new_temp_path);
    return 0;
}

int cef_create_temp_directory_in_directory(const cef_string_t* base_dir,
    const cef_string_t* prefix,
    cef_string_t* new_dir) {
    if (CEFLib::cef_create_temp_directory_in_directory)
        return CEFLib::cef_create_temp_directory_in_directory(base_dir, prefix, new_dir);
    return 0;
}

int cef_create_url(const struct _cef_urlparts_t* parts,
                              cef_string_t* url) {
    if (CEFLib::cef_create_url)
        return CEFLib::cef_create_url(parts, url);
    return 0;
}

int cef_currently_on(cef_thread_id_t threadId) {
    if (CEFLib::cef_currently_on)
        return CEFLib::cef_currently_on(threadId);
    return 0;
}

int cef_delete_file(const cef_string_t* path, int recursive) {
    if (CEFLib::cef_delete_file)
        return CEFLib::cef_delete_file(path, recursive);
    return 0;
}

cef_dictionary_value_t* cef_dictionary_value_create(void) {
    if (CEFLib::cef_dictionary_value_create)
        return CEFLib::cef_dictionary_value_create();
    return nullptr;
}

int cef_directory_exists(const cef_string_t* path) {
    if (CEFLib::cef_directory_exists)
        return CEFLib::cef_directory_exists(path);
    return 0;
}

void cef_do_message_loop_work(void) {
    if (CEFLib::cef_do_message_loop_work)
        CEFLib::cef_do_message_loop_work();
}

cef_drag_data_t* cef_drag_data_create(void) {
    if (CEFLib::cef_drag_data_create)
        return CEFLib::cef_drag_data_create();
    return nullptr;
}

int cef_end_tracing(const cef_string_t* tracing_file,
                               cef_end_tracing_callback_t* callback) {
    if (CEFLib::cef_end_tracing)
        return CEFLib::cef_end_tracing(tracing_file, callback);
    return 0;
}

int cef_execute_process(const cef_main_args_t* args,
                                   cef_app_t* application,
                                   void* windows_sandbox_info) {
    if (CEFLib::cef_execute_process)
        return CEFLib::cef_execute_process(args, application, windows_sandbox_info);
    return 0;
}

cef_string_userfree_t cef_format_url_for_security_display(const cef_string_t* origin_url) {
    if (CEFLib::cef_format_url_for_security_display)
        return CEFLib::cef_format_url_for_security_display(origin_url);
    return (cef_string_userfree_t){0};
}

cef_platform_thread_handle_t cef_get_current_platform_thread_handle(void) {
    if (CEFLib::cef_get_current_platform_thread_handle)
        return CEFLib::cef_get_current_platform_thread_handle();
    return (cef_platform_thread_handle_t){0};
}

cef_platform_thread_id_t cef_get_current_platform_thread_id(void) {
    if (CEFLib::cef_get_current_platform_thread_id)
        return CEFLib::cef_get_current_platform_thread_id();
    return (cef_platform_thread_id_t){0};
}

int cef_get_exit_code(void) {
    if (CEFLib::cef_get_exit_code)
        return CEFLib::cef_get_exit_code();
    return 0;
}

void cef_get_extensions_for_mime_type(const cef_string_t* mime_type,
                                                 cef_string_list_t extensions) {
    if (CEFLib::cef_get_extensions_for_mime_type)
        CEFLib::cef_get_extensions_for_mime_type(mime_type, extensions);
}

cef_string_userfree_t cef_get_mime_type(const cef_string_t* extension) {
    if (CEFLib::cef_get_mime_type)
        return CEFLib::cef_get_mime_type(extension);
    return (cef_string_userfree_t){0};
}

int cef_get_min_log_level(void) {
    if (CEFLib::cef_get_min_log_level)
        return CEFLib::cef_get_min_log_level();
    return 0;
}

int cef_get_path(cef_path_key_t key, cef_string_t* path) {
    if (CEFLib::cef_get_path)
        return CEFLib::cef_get_path(key, path);
    return 0;
}

int cef_get_temp_directory(cef_string_t* temp_dir) {
    if (CEFLib::cef_get_temp_directory)
        return CEFLib::cef_get_temp_directory(temp_dir);
    return 0;
}

int cef_get_vlog_level(const char* file_start, size_t N) {
    if (CEFLib::cef_get_vlog_level)
        return CEFLib::cef_get_vlog_level(file_start, N);
    return 0;
}

#ifdef PLATFORM_LINUX
XDisplay* cef_get_xdisplay(void) {
    if (CEFLib::cef_get_xdisplay)
        return CEFLib::cef_get_xdisplay();
    return nullptr;
}
#endif

cef_image_t* cef_image_create(void) {
    if (CEFLib::cef_image_create)
        return CEFLib::cef_image_create();
    return nullptr;
}

int cef_initialize(const cef_main_args_t* args,
                              const struct _cef_settings_t* settings,
                              cef_app_t* application,
                              void* windows_sandbox_info) {
    if (CEFLib::cef_initialize)
        return CEFLib::cef_initialize(args, settings, application, windows_sandbox_info);
    return 0;
}

int cef_is_cert_status_error(cef_cert_status_t status) {
    if (CEFLib::cef_is_cert_status_error)
        return CEFLib::cef_is_cert_status_error(status);
    return 0;
}

int cef_is_rtl(void) {
    if (CEFLib::cef_is_rtl)
        return CEFLib::cef_is_rtl();
    return 0;
}

int cef_launch_process(struct _cef_command_line_t* command_line) {
    if (CEFLib::cef_launch_process)
        return CEFLib::cef_launch_process(command_line);
    return 0;
}

cef_list_value_t* cef_list_value_create(void) {
    if (CEFLib::cef_list_value_create)
        return CEFLib::cef_list_value_create();
    return nullptr;
}

void cef_load_crlsets_file(const cef_string_t* path) {
    if (CEFLib::cef_load_crlsets_file)
        CEFLib::cef_load_crlsets_file(path);
}

void cef_log(const char* file,
                        int line,
                        int severity,
                        const char* message) {
    if (CEFLib::cef_log)
        CEFLib::cef_log(file, line, severity, message);
}

cef_media_router_t* cef_media_router_get_global(struct _cef_completion_callback_t* callback) {
    if (CEFLib::cef_media_router_get_global)
        return CEFLib::cef_media_router_get_global(callback);
    return nullptr;
}

cef_menu_model_t* cef_menu_model_create(struct _cef_menu_model_delegate_t* delegate) {
    if (CEFLib::cef_menu_model_create)
        return CEFLib::cef_menu_model_create(delegate);
    return nullptr;
}

int64_t cef_now_from_system_trace_time(void) {
    if (CEFLib::cef_now_from_system_trace_time)
        return CEFLib::cef_now_from_system_trace_time();
    return (int64_t){0};
}

struct _cef_value_t* cef_parse_json(const cef_string_t* json_string,
    cef_json_parser_options_t options) {
    if (CEFLib::cef_parse_json)
        return CEFLib::cef_parse_json(json_string, options);
    return nullptr;
}

struct _cef_value_t* cef_parse_json_buffer(const void* json,
    size_t json_size,
    cef_json_parser_options_t options) {
    if (CEFLib::cef_parse_json_buffer)
        return CEFLib::cef_parse_json_buffer(json, json_size, options);
    return nullptr;
}

struct _cef_value_t* cef_parse_jsonand_return_error(const cef_string_t* json_string,
    cef_json_parser_options_t options,
    cef_string_t* error_msg_out) {
    if (CEFLib::cef_parse_jsonand_return_error)
        return CEFLib::cef_parse_jsonand_return_error(json_string, options, error_msg_out);
    return nullptr;
}

int cef_parse_url(const cef_string_t* url,
                             struct _cef_urlparts_t* parts) {
    if (CEFLib::cef_parse_url)
        return CEFLib::cef_parse_url(url, parts);
    return 0;
}

cef_post_data_t* cef_post_data_create(void) {
    if (CEFLib::cef_post_data_create)
        return CEFLib::cef_post_data_create();
    return nullptr;
}

cef_post_data_element_t* cef_post_data_element_create(void) {
    if (CEFLib::cef_post_data_element_create)
        return CEFLib::cef_post_data_element_create();
    return nullptr;
}

int cef_post_delayed_task(cef_thread_id_t threadId,
                                     cef_task_t* task,
                                     int64_t delay_ms) {
    if (CEFLib::cef_post_delayed_task)
        return CEFLib::cef_post_delayed_task(threadId, task, delay_ms);
    return 0;
}

int cef_post_task(cef_thread_id_t threadId, cef_task_t* task) {
    if (CEFLib::cef_post_task)
        return CEFLib::cef_post_task(threadId, task);
    return 0;
}

cef_preference_manager_t* cef_preference_manager_get_global(void) {
    if (CEFLib::cef_preference_manager_get_global)
        return CEFLib::cef_preference_manager_get_global();
    return nullptr;
}

cef_print_settings_t* cef_print_settings_create(void) {
    if (CEFLib::cef_print_settings_create)
        return CEFLib::cef_print_settings_create();
    return nullptr;
}

cef_process_message_t* cef_process_message_create(const cef_string_t* name) {
    if (CEFLib::cef_process_message_create)
        return CEFLib::cef_process_message_create(name);
    return nullptr;
}

void cef_quit_message_loop(void) {
    if (CEFLib::cef_quit_message_loop)
        CEFLib::cef_quit_message_loop();
}

int cef_register_extension(const cef_string_t* extension_name,
                                      const cef_string_t* javascript_code,
                                      cef_v8handler_t* handler) {
    if (CEFLib::cef_register_extension)
        return CEFLib::cef_register_extension(extension_name, javascript_code, handler);
    return 0;
}

int cef_register_scheme_handler_factory(const cef_string_t* scheme_name,
    const cef_string_t* domain_name,
    cef_scheme_handler_factory_t* factory) {
    if (CEFLib::cef_register_scheme_handler_factory)
        return CEFLib::cef_register_scheme_handler_factory(scheme_name, domain_name, factory);
    return 0;
}

int cef_remove_cross_origin_whitelist_entry(const cef_string_t* source_origin,
    const cef_string_t* target_protocol,
    const cef_string_t* target_domain,
    int allow_target_subdomains) {
    if (CEFLib::cef_remove_cross_origin_whitelist_entry)
        return CEFLib::cef_remove_cross_origin_whitelist_entry(source_origin, target_protocol, target_domain, allow_target_subdomains);
    return 0;
}

cef_request_context_t* cef_request_context_create_context(const struct _cef_request_context_settings_t* settings,
    struct _cef_request_context_handler_t* handler) {
    if (CEFLib::cef_request_context_create_context)
        return CEFLib::cef_request_context_create_context(settings, handler);
    return nullptr;
}

cef_request_context_t* cef_request_context_get_global_context(void) {
    if (CEFLib::cef_request_context_get_global_context)
        return CEFLib::cef_request_context_get_global_context();
    return nullptr;
}

cef_request_t* cef_request_create(void) {
    if (CEFLib::cef_request_create)
        return CEFLib::cef_request_create();
    return nullptr;
}

int cef_resolve_url(const cef_string_t* base_url,
                               const cef_string_t* relative_url,
                               cef_string_t* resolved_url) {
    if (CEFLib::cef_resolve_url)
        return CEFLib::cef_resolve_url(base_url, relative_url, resolved_url);
    return 0;
}

cef_resource_bundle_t* cef_resource_bundle_get_global(void) {
    if (CEFLib::cef_resource_bundle_get_global)
        return CEFLib::cef_resource_bundle_get_global();
    return nullptr;
}

cef_response_t* cef_response_create(void) {
    if (CEFLib::cef_response_create)
        return CEFLib::cef_response_create();
    return nullptr;
}

void cef_run_message_loop(void) {
    if (CEFLib::cef_run_message_loop)
        CEFLib::cef_run_message_loop();
}

void cef_server_create(const cef_string_t* address,
                                  uint16_t port,
                                  int backlog,
                                  struct _cef_server_handler_t* handler) {
    if (CEFLib::cef_server_create)
        CEFLib::cef_server_create(address, port, backlog, handler);
}

void cef_set_crash_key_value(const cef_string_t* key,
                                        const cef_string_t* value) {
    if (CEFLib::cef_set_crash_key_value)
        CEFLib::cef_set_crash_key_value(key, value);
}

void cef_set_osmodal_loop(int osModalLoop) {
    if (CEFLib::cef_set_osmodal_loop)
        CEFLib::cef_set_osmodal_loop(osModalLoop);
}

cef_shared_process_message_builder_t* cef_shared_process_message_builder_create(const cef_string_t* name,
                                          size_t byte_size) {
    if (CEFLib::cef_shared_process_message_builder_create)
        return CEFLib::cef_shared_process_message_builder_create(name, byte_size);
    return nullptr;
}

void cef_shutdown(void) {
    if (CEFLib::cef_shutdown)
        CEFLib::cef_shutdown();
}

cef_stream_reader_t* cef_stream_reader_create_for_data(void* data,
                                                                  size_t size) {
    if (CEFLib::cef_stream_reader_create_for_data)
        return CEFLib::cef_stream_reader_create_for_data(data, size);
    return nullptr;
}

cef_stream_reader_t* cef_stream_reader_create_for_file(const cef_string_t* fileName) {
    if (CEFLib::cef_stream_reader_create_for_file)
        return CEFLib::cef_stream_reader_create_for_file(fileName);
    return nullptr;
}

cef_stream_reader_t* cef_stream_reader_create_for_handler(cef_read_handler_t* handler) {
    if (CEFLib::cef_stream_reader_create_for_handler)
        return CEFLib::cef_stream_reader_create_for_handler(handler);
    return nullptr;
}

cef_stream_writer_t* cef_stream_writer_create_for_file(const cef_string_t* fileName) {
    if (CEFLib::cef_stream_writer_create_for_file)
        return CEFLib::cef_stream_writer_create_for_file(fileName);
    return nullptr;
}

cef_stream_writer_t* cef_stream_writer_create_for_handler(cef_write_handler_t* handler) {
    if (CEFLib::cef_stream_writer_create_for_handler)
        return CEFLib::cef_stream_writer_create_for_handler(handler);
    return nullptr;
}

int cef_string_ascii_to_utf16(const char* src,
                                         size_t src_len,
                                         cef_string_utf16_t* output) {
    if (CEFLib::cef_string_ascii_to_utf16)
        return CEFLib::cef_string_ascii_to_utf16(src, src_len, output);
    return 0;
}

int cef_string_ascii_to_wide(const char* src,
                                        size_t src_len,
                                        cef_string_wide_t* output) {
    if (CEFLib::cef_string_ascii_to_wide)
        return CEFLib::cef_string_ascii_to_wide(src, src_len, output);
    return 0;
}

cef_string_list_t cef_string_list_alloc(void) {
    if (CEFLib::cef_string_list_alloc)
        return CEFLib::cef_string_list_alloc();
    return (cef_string_list_t){0};
}

void cef_string_list_append(cef_string_list_t list,
                                       const cef_string_t* value) {
    if (CEFLib::cef_string_list_append)
        CEFLib::cef_string_list_append(list, value);
}

void cef_string_list_clear(cef_string_list_t list) {
    if (CEFLib::cef_string_list_clear)
        CEFLib::cef_string_list_clear(list);
}

cef_string_list_t cef_string_list_copy(cef_string_list_t list) {
    if (CEFLib::cef_string_list_copy)
        return CEFLib::cef_string_list_copy(list);
    return (cef_string_list_t){0};
}

void cef_string_list_free(cef_string_list_t list) {
    if (CEFLib::cef_string_list_free)
        CEFLib::cef_string_list_free(list);
}

size_t cef_string_list_size(cef_string_list_t list) {
    if (CEFLib::cef_string_list_size)
        return CEFLib::cef_string_list_size(list);
    return 0;
}

int cef_string_list_value(cef_string_list_t list,
                                     size_t index,
                                     cef_string_t* value) {
    if (CEFLib::cef_string_list_value)
        return CEFLib::cef_string_list_value(list, index, value);
    return 0;
}

cef_string_map_t cef_string_map_alloc(void) {
    if (CEFLib::cef_string_map_alloc)
        return CEFLib::cef_string_map_alloc();
    return (cef_string_map_t){0};
}

int cef_string_map_append(cef_string_map_t map,
                                     const cef_string_t* key,
                                     const cef_string_t* value) {
    if (CEFLib::cef_string_map_append)
        return CEFLib::cef_string_map_append(map, key, value);
    return 0;
}

void cef_string_map_clear(cef_string_map_t map) {
    if (CEFLib::cef_string_map_clear)
        CEFLib::cef_string_map_clear(map);
}

int cef_string_map_find(cef_string_map_t map,
                                   const cef_string_t* key,
                                   cef_string_t* value) {
    if (CEFLib::cef_string_map_find)
        return CEFLib::cef_string_map_find(map, key, value);
    return 0;
}

void cef_string_map_free(cef_string_map_t map) {
    if (CEFLib::cef_string_map_free)
        CEFLib::cef_string_map_free(map);
}

int cef_string_map_key(cef_string_map_t map,
                                  size_t index,
                                  cef_string_t* key) {
    if (CEFLib::cef_string_map_key)
        return CEFLib::cef_string_map_key(map, index, key);
    return 0;
}

size_t cef_string_map_size(cef_string_map_t map) {
    if (CEFLib::cef_string_map_size)
        return CEFLib::cef_string_map_size(map);
    return 0;
}

int cef_string_map_value(cef_string_map_t map,
                                    size_t index,
                                    cef_string_t* value) {
    if (CEFLib::cef_string_map_value)
        return CEFLib::cef_string_map_value(map, index, value);
    return 0;
}

cef_string_multimap_t cef_string_multimap_alloc(void) {
    if (CEFLib::cef_string_multimap_alloc)
        return CEFLib::cef_string_multimap_alloc();
    return (cef_string_multimap_t){0};
}

int cef_string_multimap_append(cef_string_multimap_t map,
                                          const cef_string_t* key,
                                          const cef_string_t* value) {
    if (CEFLib::cef_string_multimap_append)
        return CEFLib::cef_string_multimap_append(map, key, value);
    return 0;
}

void cef_string_multimap_clear(cef_string_multimap_t map) {
    if (CEFLib::cef_string_multimap_clear)
        CEFLib::cef_string_multimap_clear(map);
}

int cef_string_multimap_enumerate(cef_string_multimap_t map,
                                             const cef_string_t* key,
                                             size_t value_index,
                                             cef_string_t* value) {
    if (CEFLib::cef_string_multimap_enumerate)
        return CEFLib::cef_string_multimap_enumerate(map, key, value_index, value);
    return 0;
}

size_t cef_string_multimap_find_count(cef_string_multimap_t map,
                                                 const cef_string_t* key) {
    if (CEFLib::cef_string_multimap_find_count)
        return CEFLib::cef_string_multimap_find_count(map, key);
    return 0;
}

void cef_string_multimap_free(cef_string_multimap_t map) {
    if (CEFLib::cef_string_multimap_free)
        CEFLib::cef_string_multimap_free(map);
}

int cef_string_multimap_key(cef_string_multimap_t map,
                                       size_t index,
                                       cef_string_t* key) {
    if (CEFLib::cef_string_multimap_key)
        return CEFLib::cef_string_multimap_key(map, index, key);
    return 0;
}

size_t cef_string_multimap_size(cef_string_multimap_t map) {
    if (CEFLib::cef_string_multimap_size)
        return CEFLib::cef_string_multimap_size(map);
    return 0;
}

int cef_string_multimap_value(cef_string_multimap_t map,
                                         size_t index,
                                         cef_string_t* value) {
    if (CEFLib::cef_string_multimap_value)
        return CEFLib::cef_string_multimap_value(map, index, value);
    return 0;
}

cef_string_userfree_utf16_t cef_string_userfree_utf16_alloc(void) {
    if (CEFLib::cef_string_userfree_utf16_alloc)
        return CEFLib::cef_string_userfree_utf16_alloc();
    return (cef_string_userfree_utf16_t){0};
}

void cef_string_userfree_utf16_free(cef_string_userfree_utf16_t str) {
    if (CEFLib::cef_string_userfree_utf16_free)
        CEFLib::cef_string_userfree_utf16_free(str);
}

cef_string_userfree_utf8_t cef_string_userfree_utf8_alloc(void) {
    if (CEFLib::cef_string_userfree_utf8_alloc)
        return CEFLib::cef_string_userfree_utf8_alloc();
    return (cef_string_userfree_utf8_t){0};
}

void cef_string_userfree_utf8_free(cef_string_userfree_utf8_t str) {
    if (CEFLib::cef_string_userfree_utf8_free)
        CEFLib::cef_string_userfree_utf8_free(str);
}

cef_string_userfree_wide_t cef_string_userfree_wide_alloc(void) {
    if (CEFLib::cef_string_userfree_wide_alloc)
        return CEFLib::cef_string_userfree_wide_alloc();
    return (cef_string_userfree_wide_t){0};
}

void cef_string_userfree_wide_free(cef_string_userfree_wide_t str) {
    if (CEFLib::cef_string_userfree_wide_free)
        CEFLib::cef_string_userfree_wide_free(str);
}

void cef_string_utf16_clear(cef_string_utf16_t* str) {
    if (CEFLib::cef_string_utf16_clear)
        CEFLib::cef_string_utf16_clear(str);
}

int cef_string_utf16_cmp(const cef_string_utf16_t* str1,
                                    const cef_string_utf16_t* str2) {
    if (CEFLib::cef_string_utf16_cmp)
        return CEFLib::cef_string_utf16_cmp(str1, str2);
    return 0;
}

int cef_string_utf16_set(const char16_t* src,
                                    size_t src_len,
                                    cef_string_utf16_t* output,
                                    int copy) {
    if (CEFLib::cef_string_utf16_set)
        return CEFLib::cef_string_utf16_set(src, src_len, output, copy);
    return 0;
}

int cef_string_utf16_to_lower(const char16_t* src,
                                         size_t src_len,
                                         cef_string_utf16_t* output) {
    if (CEFLib::cef_string_utf16_to_lower)
        return CEFLib::cef_string_utf16_to_lower(src, src_len, output);
    return 0;
}

int cef_string_utf16_to_upper(const char16_t* src,
                                         size_t src_len,
                                         cef_string_utf16_t* output) {
    if (CEFLib::cef_string_utf16_to_upper)
        return CEFLib::cef_string_utf16_to_upper(src, src_len, output);
    return 0;
}

int cef_string_utf16_to_utf8(const char16_t* src,
                                        size_t src_len,
                                        cef_string_utf8_t* output) {
    if (CEFLib::cef_string_utf16_to_utf8)
        return CEFLib::cef_string_utf16_to_utf8(src, src_len, output);
    return 0;
}

int cef_string_utf16_to_wide(const char16_t* src,
                                        size_t src_len,
                                        cef_string_wide_t* output) {
    if (CEFLib::cef_string_utf16_to_wide)
        return CEFLib::cef_string_utf16_to_wide(src, src_len, output);
    return 0;
}

void cef_string_utf8_clear(cef_string_utf8_t* str) {
    if (CEFLib::cef_string_utf8_clear)
        CEFLib::cef_string_utf8_clear(str);
}

int cef_string_utf8_cmp(const cef_string_utf8_t* str1,
                                   const cef_string_utf8_t* str2) {
    if (CEFLib::cef_string_utf8_cmp)
        return CEFLib::cef_string_utf8_cmp(str1, str2);
    return 0;
}

int cef_string_utf8_set(const char* src,
                                   size_t src_len,
                                   cef_string_utf8_t* output,
                                   int copy) {
    if (CEFLib::cef_string_utf8_set)
        return CEFLib::cef_string_utf8_set(src, src_len, output, copy);
    return 0;
}

int cef_string_utf8_to_utf16(const char* src,
                                        size_t src_len,
                                        cef_string_utf16_t* output) {
    if (CEFLib::cef_string_utf8_to_utf16)
        return CEFLib::cef_string_utf8_to_utf16(src, src_len, output);
    return 0;
}

int cef_string_utf8_to_wide(const char* src,
                                       size_t src_len,
                                       cef_string_wide_t* output) {
    if (CEFLib::cef_string_utf8_to_wide)
        return CEFLib::cef_string_utf8_to_wide(src, src_len, output);
    return 0;
}

void cef_string_wide_clear(cef_string_wide_t* str) {
    if (CEFLib::cef_string_wide_clear)
        CEFLib::cef_string_wide_clear(str);
}

int cef_string_wide_cmp(const cef_string_wide_t* str1,
                                   const cef_string_wide_t* str2) {
    if (CEFLib::cef_string_wide_cmp)
        return CEFLib::cef_string_wide_cmp(str1, str2);
    return 0;
}

int cef_string_wide_set(const wchar_t* src,
                                   size_t src_len,
                                   cef_string_wide_t* output,
                                   int copy) {
    if (CEFLib::cef_string_wide_set)
        return CEFLib::cef_string_wide_set(src, src_len, output, copy);
    return 0;
}

int cef_string_wide_to_utf16(const wchar_t* src,
                                        size_t src_len,
                                        cef_string_utf16_t* output) {
    if (CEFLib::cef_string_wide_to_utf16)
        return CEFLib::cef_string_wide_to_utf16(src, src_len, output);
    return 0;
}

int cef_string_wide_to_utf8(const wchar_t* src,
                                       size_t src_len,
                                       cef_string_utf8_t* output) {
    if (CEFLib::cef_string_wide_to_utf8)
        return CEFLib::cef_string_wide_to_utf8(src, src_len, output);
    return 0;
}

cef_task_runner_t* cef_task_runner_get_for_current_thread(void) {
    if (CEFLib::cef_task_runner_get_for_current_thread)
        return CEFLib::cef_task_runner_get_for_current_thread();
    return nullptr;
}

cef_task_runner_t* cef_task_runner_get_for_thread(cef_thread_id_t threadId) {
    if (CEFLib::cef_task_runner_get_for_thread)
        return CEFLib::cef_task_runner_get_for_thread(threadId);
    return nullptr;
}

cef_thread_t* cef_thread_create(const cef_string_t* display_name,
    cef_thread_priority_t priority,
    cef_message_loop_type_t message_loop_type,
    int stoppable,
    cef_com_init_mode_t com_init_mode) {
    if (CEFLib::cef_thread_create)
        return CEFLib::cef_thread_create(display_name, priority, message_loop_type, stoppable, com_init_mode);
    return nullptr;
}

int cef_time_delta(const cef_time_t* cef_time1,
                              const cef_time_t* cef_time2,
                              long long* delta) {
    if (CEFLib::cef_time_delta)
        return CEFLib::cef_time_delta(cef_time1, cef_time2, delta);
    return 0;
}

int cef_time_from_basetime(const cef_basetime_t from,
                                      cef_time_t* to) {
    if (CEFLib::cef_time_from_basetime)
        return CEFLib::cef_time_from_basetime(from, to);
    return 0;
}

int cef_time_from_doublet(double time, cef_time_t* cef_time) {
    if (CEFLib::cef_time_from_doublet)
        return CEFLib::cef_time_from_doublet(time, cef_time);
    return 0;
}

int cef_time_from_timet(time_t time, cef_time_t* cef_time) {
    if (CEFLib::cef_time_from_timet)
        return CEFLib::cef_time_from_timet(time, cef_time);
    return 0;
}

int cef_time_now(cef_time_t* cef_time) {
    if (CEFLib::cef_time_now)
        return CEFLib::cef_time_now(cef_time);
    return 0;
}

int cef_time_to_basetime(const cef_time_t* from, cef_basetime_t* to) {
    if (CEFLib::cef_time_to_basetime)
        return CEFLib::cef_time_to_basetime(from, to);
    return 0;
}

int cef_time_to_doublet(const cef_time_t* cef_time, double* time) {
    if (CEFLib::cef_time_to_doublet)
        return CEFLib::cef_time_to_doublet(cef_time, time);
    return 0;
}

int cef_time_to_timet(const cef_time_t* cef_time, time_t* time) {
    if (CEFLib::cef_time_to_timet)
        return CEFLib::cef_time_to_timet(cef_time, time);
    return 0;
}

void cef_trace_counter(const char* category,
                                  const char* name,
                                  const char* value1_name,
                                  uint64_t value1_val,
                                  const char* value2_name,
                                  uint64_t value2_val) {
    if (CEFLib::cef_trace_counter)
        CEFLib::cef_trace_counter(category, name, value1_name, value1_val, value2_name, value2_val);
}

void cef_trace_counter_id(const char* category,
                                     const char* name,
                                     uint64_t id,
                                     const char* value1_name,
                                     uint64_t value1_val,
                                     const char* value2_name,
                                     uint64_t value2_val) {
    if (CEFLib::cef_trace_counter_id)
        CEFLib::cef_trace_counter_id(category, name, id, value1_name, value1_val, value2_name, value2_val);
}

void cef_trace_event_async_begin(const char* category,
                                            const char* name,
                                            uint64_t id,
                                            const char* arg1_name,
                                            uint64_t arg1_val,
                                            const char* arg2_name,
                                            uint64_t arg2_val) {
    if (CEFLib::cef_trace_event_async_begin)
        CEFLib::cef_trace_event_async_begin(category, name, id, arg1_name, arg1_val, arg2_name, arg2_val);
}

void cef_trace_event_async_end(const char* category,
                                          const char* name,
                                          uint64_t id,
                                          const char* arg1_name,
                                          uint64_t arg1_val,
                                          const char* arg2_name,
                                          uint64_t arg2_val) {
    if (CEFLib::cef_trace_event_async_end)
        CEFLib::cef_trace_event_async_end(category, name, id, arg1_name, arg1_val, arg2_name, arg2_val);
}

void cef_trace_event_async_step_into(const char* category,
                                                const char* name,
                                                uint64_t id,
                                                uint64_t step,
                                                const char* arg1_name,
                                                uint64_t arg1_val) {
    if (CEFLib::cef_trace_event_async_step_into)
        CEFLib::cef_trace_event_async_step_into(category, name, id, step, arg1_name, arg1_val);
}

void cef_trace_event_async_step_past(const char* category,
                                                const char* name,
                                                uint64_t id,
                                                uint64_t step,
                                                const char* arg1_name,
                                                uint64_t arg1_val) {
    if (CEFLib::cef_trace_event_async_step_past)
        CEFLib::cef_trace_event_async_step_past(category, name, id, step, arg1_name, arg1_val);
}

void cef_trace_event_begin(const char* category,
                                      const char* name,
                                      const char* arg1_name,
                                      uint64_t arg1_val,
                                      const char* arg2_name,
                                      uint64_t arg2_val) {
    if (CEFLib::cef_trace_event_begin)
        CEFLib::cef_trace_event_begin(category, name, arg1_name, arg1_val, arg2_name, arg2_val);
}

void cef_trace_event_end(const char* category,
                                    const char* name,
                                    const char* arg1_name,
                                    uint64_t arg1_val,
                                    const char* arg2_name,
                                    uint64_t arg2_val) {
    if (CEFLib::cef_trace_event_end)
        CEFLib::cef_trace_event_end(category, name, arg1_name, arg1_val, arg2_name, arg2_val);
}

void cef_trace_event_instant(const char* category,
                                        const char* name,
                                        const char* arg1_name,
                                        uint64_t arg1_val,
                                        const char* arg2_name,
                                        uint64_t arg2_val) {
    if (CEFLib::cef_trace_event_instant)
        CEFLib::cef_trace_event_instant(category, name, arg1_name, arg1_val, arg2_name, arg2_val);
}

cef_string_userfree_t cef_uridecode(const cef_string_t* text,
              int convert_to_utf8,
              cef_uri_unescape_rule_t unescape_rule) {
    if (CEFLib::cef_uridecode)
        return CEFLib::cef_uridecode(text, convert_to_utf8, unescape_rule);
    return (cef_string_userfree_t){0};
}

cef_string_userfree_t cef_uriencode(const cef_string_t* text,
                                               int use_plus) {
    if (CEFLib::cef_uriencode)
        return CEFLib::cef_uriencode(text, use_plus);
    return (cef_string_userfree_t){0};
}

cef_urlrequest_t* cef_urlrequest_create(struct _cef_request_t* request,
    struct _cef_urlrequest_client_t* client,
    struct _cef_request_context_t* request_context) {
    if (CEFLib::cef_urlrequest_create)
        return CEFLib::cef_urlrequest_create(request, client, request_context);
    return nullptr;
}

cef_v8context_t* cef_v8context_get_current_context(void) {
    if (CEFLib::cef_v8context_get_current_context)
        return CEFLib::cef_v8context_get_current_context();
    return nullptr;
}

cef_v8context_t* cef_v8context_get_entered_context(void) {
    if (CEFLib::cef_v8context_get_entered_context)
        return CEFLib::cef_v8context_get_entered_context();
    return nullptr;
}

int cef_v8context_in_context(void) {
    if (CEFLib::cef_v8context_in_context)
        return CEFLib::cef_v8context_in_context();
    return 0;
}

cef_v8stack_trace_t* cef_v8stack_trace_get_current(int frame_limit) {
    if (CEFLib::cef_v8stack_trace_get_current)
        return CEFLib::cef_v8stack_trace_get_current(frame_limit);
    return nullptr;
}

cef_v8value_t* cef_v8value_create_array(int length) {
    if (CEFLib::cef_v8value_create_array)
        return CEFLib::cef_v8value_create_array(length);
    return nullptr;
}

cef_v8value_t* cef_v8value_create_array_buffer(void* buffer,
    size_t length,
    cef_v8array_buffer_release_callback_t* release_callback) {
    if (CEFLib::cef_v8value_create_array_buffer)
        return CEFLib::cef_v8value_create_array_buffer(buffer, length, release_callback);
    return nullptr;
}

cef_v8value_t* cef_v8value_create_bool(int value) {
    if (CEFLib::cef_v8value_create_bool)
        return CEFLib::cef_v8value_create_bool(value);
    return nullptr;
}

cef_v8value_t* cef_v8value_create_date(cef_basetime_t date) {
    if (CEFLib::cef_v8value_create_date)
        return CEFLib::cef_v8value_create_date(date);
    return nullptr;
}

cef_v8value_t* cef_v8value_create_double(double value) {
    if (CEFLib::cef_v8value_create_double)
        return CEFLib::cef_v8value_create_double(value);
    return nullptr;
}

cef_v8value_t* cef_v8value_create_function(const cef_string_t* name,
                                                      cef_v8handler_t* handler) {
    if (CEFLib::cef_v8value_create_function)
        return CEFLib::cef_v8value_create_function(name, handler);
    return nullptr;
}

cef_v8value_t* cef_v8value_create_int(int32_t value) {
    if (CEFLib::cef_v8value_create_int)
        return CEFLib::cef_v8value_create_int(value);
    return nullptr;
}

cef_v8value_t* cef_v8value_create_null(void) {
    if (CEFLib::cef_v8value_create_null)
        return CEFLib::cef_v8value_create_null();
    return nullptr;
}

cef_v8value_t* cef_v8value_create_object(cef_v8accessor_t* accessor,
    cef_v8interceptor_t* interceptor) {
    if (CEFLib::cef_v8value_create_object)
        return CEFLib::cef_v8value_create_object(accessor, interceptor);
    return nullptr;
}

cef_v8value_t* cef_v8value_create_promise(void) {
    if (CEFLib::cef_v8value_create_promise)
        return CEFLib::cef_v8value_create_promise();
    return nullptr;
}

cef_v8value_t* cef_v8value_create_string(const cef_string_t* value) {
    if (CEFLib::cef_v8value_create_string)
        return CEFLib::cef_v8value_create_string(value);
    return nullptr;
}

cef_v8value_t* cef_v8value_create_uint(uint32_t value) {
    if (CEFLib::cef_v8value_create_uint)
        return CEFLib::cef_v8value_create_uint(value);
    return nullptr;
}

cef_v8value_t* cef_v8value_create_undefined(void) {
    if (CEFLib::cef_v8value_create_undefined)
        return CEFLib::cef_v8value_create_undefined();
    return nullptr;
}

cef_value_t* cef_value_create(void) {
    if (CEFLib::cef_value_create)
        return CEFLib::cef_value_create();
    return nullptr;
}

cef_waitable_event_t* cef_waitable_event_create(int automatic_reset,
    int initially_signaled) {
    if (CEFLib::cef_waitable_event_create)
        return CEFLib::cef_waitable_event_create(automatic_reset, initially_signaled);
    return nullptr;
}

cef_string_userfree_t cef_write_json(struct _cef_value_t* node, cef_json_writer_options_t options) {
    if (CEFLib::cef_write_json)
        return CEFLib::cef_write_json(node, options);
    return (cef_string_userfree_t){0};
}

cef_xml_reader_t* cef_xml_reader_create(struct _cef_stream_reader_t* stream,
    cef_xml_encoding_type_t encodingType,
    const cef_string_t* URI) {
    if (CEFLib::cef_xml_reader_create)
        return CEFLib::cef_xml_reader_create(stream, encodingType, URI);
    return nullptr;
}

int cef_zip_directory(const cef_string_t* src_dir,
                                 const cef_string_t* dest_file,
                                 int include_hidden_files) {
    if (CEFLib::cef_zip_directory)
        return CEFLib::cef_zip_directory(src_dir, dest_file, include_hidden_files);
    return 0;
}

cef_zip_reader_t* cef_zip_reader_create(struct _cef_stream_reader_t* stream) {
    if (CEFLib::cef_zip_reader_create)
        return CEFLib::cef_zip_reader_create(stream);
    return nullptr;
}

} // extern "C"
