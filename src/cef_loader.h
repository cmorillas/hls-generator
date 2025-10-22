#ifndef CEF_LOADER_H
#define CEF_LOADER_H

#include <string>

// Forward declarations for CEF types
#include "include/capi/cef_app_capi.h"
#include "include/capi/cef_client_capi.h"
#include "include/capi/cef_trace_capi.h"
#include "include/capi/cef_thread_capi.h"
#include "include/capi/cef_urlrequest_capi.h"
#include "include/capi/cef_waitable_event_capi.h"
#include "include/capi/cef_xml_reader_capi.h"
#include "include/capi/cef_zip_reader_capi.h"
#include "include/capi/cef_resource_bundle_capi.h"
#include "include/capi/cef_shared_process_message_builder_capi.h"
#include "include/capi/cef_server_capi.h"
#include "include/internal/cef_thread_internal.h"

#ifdef PLATFORM_LINUX
#include <X11/Xlib.h>
typedef Display XDisplay;
#endif

namespace CEFLib {
    // CEF function pointers (auto-generated - 176 functions)
    extern int(*cef_add_cross_origin_whitelist_entry)(const cef_string_t* source_origin, const cef_string_t* target_protocol, const cef_string_t* target_domain, int allow_target_subdomains);
    extern struct _cef_binary_value_t*(*cef_base64decode)(const cef_string_t* data);
    extern cef_string_userfree_t(*cef_base64encode)(const void* data, size_t data_size);
    extern cef_basetime_t(*cef_basetime_now)(void);
    extern int(*cef_begin_tracing)(const cef_string_t* categories, struct _cef_completion_callback_t* callback);
    extern cef_binary_value_t*(*cef_binary_value_create)(const void* data, size_t data_size);
    extern int(*cef_browser_host_create_browser)(const cef_window_info_t* windowInfo, struct _cef_client_t* client, const cef_string_t* url, const struct _cef_browser_settings_t* settings, struct _cef_dictionary_value_t* extra_info, struct _cef_request_context_t* request_context);
    extern cef_browser_t*(*cef_browser_host_create_browser_sync)(const cef_window_info_t* windowInfo, struct _cef_client_t* client, const cef_string_t* url, const struct _cef_browser_settings_t* settings, struct _cef_dictionary_value_t* extra_info, struct _cef_request_context_t* request_context);
    extern int(*cef_clear_cross_origin_whitelist)(void);
    extern int(*cef_clear_scheme_handler_factories)(void);
    extern cef_command_line_t*(*cef_command_line_create)(void);
    extern cef_command_line_t*(*cef_command_line_get_global)(void);
    extern cef_cookie_manager_t*(*cef_cookie_manager_get_global_manager)(struct _cef_completion_callback_t* callback);
    extern int(*cef_crash_reporting_enabled)(void);
    extern cef_request_context_t*(*cef_create_context_shared)(cef_request_context_t* other, struct _cef_request_context_handler_t* handler);
    extern int(*cef_create_directory)(const cef_string_t* full_path);
    extern int(*cef_create_new_temp_directory)(const cef_string_t* prefix, cef_string_t* new_temp_path);
    extern int(*cef_create_temp_directory_in_directory)(const cef_string_t* base_dir, const cef_string_t* prefix, cef_string_t* new_dir);
    extern int(*cef_create_url)(const struct _cef_urlparts_t* parts, cef_string_t* url);
    extern int(*cef_currently_on)(cef_thread_id_t threadId);
    extern int(*cef_delete_file)(const cef_string_t* path, int recursive);
    extern cef_dictionary_value_t*(*cef_dictionary_value_create)(void);
    extern int(*cef_directory_exists)(const cef_string_t* path);
    extern void(*cef_do_message_loop_work)(void);
    extern cef_drag_data_t*(*cef_drag_data_create)(void);
    extern int(*cef_end_tracing)(const cef_string_t* tracing_file, cef_end_tracing_callback_t* callback);
    extern int(*cef_execute_process)(const cef_main_args_t* args, cef_app_t* application, void* windows_sandbox_info);
    extern cef_string_userfree_t(*cef_format_url_for_security_display)(const cef_string_t* origin_url);
    extern cef_platform_thread_handle_t(*cef_get_current_platform_thread_handle)(void);
    extern cef_platform_thread_id_t(*cef_get_current_platform_thread_id)(void);
    extern int(*cef_get_exit_code)(void);
    extern void(*cef_get_extensions_for_mime_type)(const cef_string_t* mime_type, cef_string_list_t extensions);
    extern cef_string_userfree_t(*cef_get_mime_type)(const cef_string_t* extension);
    extern int(*cef_get_min_log_level)(void);
    extern int(*cef_get_path)(cef_path_key_t key, cef_string_t* path);
    extern int(*cef_get_temp_directory)(cef_string_t* temp_dir);
    extern int(*cef_get_vlog_level)(const char* file_start, size_t N);
#ifdef PLATFORM_LINUX
    extern XDisplay*(*cef_get_xdisplay)(void);
#endif
    extern cef_image_t*(*cef_image_create)(void);
    extern int(*cef_initialize)(const cef_main_args_t* args, const struct _cef_settings_t* settings, cef_app_t* application, void* windows_sandbox_info);
    extern int(*cef_is_cert_status_error)(cef_cert_status_t status);
    extern int(*cef_is_rtl)(void);
    extern int(*cef_launch_process)(struct _cef_command_line_t* command_line);
    extern cef_list_value_t*(*cef_list_value_create)(void);
    extern void(*cef_load_crlsets_file)(const cef_string_t* path);
    extern void(*cef_log)(const char* file, int line, int severity, const char* message);
    extern cef_media_router_t*(*cef_media_router_get_global)(struct _cef_completion_callback_t* callback);
    extern cef_menu_model_t*(*cef_menu_model_create)(struct _cef_menu_model_delegate_t* delegate);
    extern int64_t(*cef_now_from_system_trace_time)(void);
    extern struct _cef_value_t*(*cef_parse_json)(const cef_string_t* json_string, cef_json_parser_options_t options);
    extern struct _cef_value_t*(*cef_parse_json_buffer)(const void* json, size_t json_size, cef_json_parser_options_t options);
    extern struct _cef_value_t*(*cef_parse_jsonand_return_error)(const cef_string_t* json_string, cef_json_parser_options_t options, cef_string_t* error_msg_out);
    extern int(*cef_parse_url)(const cef_string_t* url, struct _cef_urlparts_t* parts);
    extern cef_post_data_t*(*cef_post_data_create)(void);
    extern cef_post_data_element_t*(*cef_post_data_element_create)(void);
    extern int(*cef_post_delayed_task)(cef_thread_id_t threadId, cef_task_t* task, int64_t delay_ms);
    extern int(*cef_post_task)(cef_thread_id_t threadId, cef_task_t* task);
    extern cef_preference_manager_t*(*cef_preference_manager_get_global)(void);
    extern cef_print_settings_t*(*cef_print_settings_create)(void);
    extern cef_process_message_t*(*cef_process_message_create)(const cef_string_t* name);
    extern void(*cef_quit_message_loop)(void);
    extern int(*cef_register_extension)(const cef_string_t* extension_name, const cef_string_t* javascript_code, cef_v8handler_t* handler);
    extern int(*cef_register_scheme_handler_factory)(const cef_string_t* scheme_name, const cef_string_t* domain_name, cef_scheme_handler_factory_t* factory);
    extern int(*cef_remove_cross_origin_whitelist_entry)(const cef_string_t* source_origin, const cef_string_t* target_protocol, const cef_string_t* target_domain, int allow_target_subdomains);
    extern cef_request_context_t*(*cef_request_context_create_context)(const struct _cef_request_context_settings_t* settings, struct _cef_request_context_handler_t* handler);
    extern cef_request_context_t*(*cef_request_context_get_global_context)(void);
    extern cef_request_t*(*cef_request_create)(void);
    extern int(*cef_resolve_url)(const cef_string_t* base_url, const cef_string_t* relative_url, cef_string_t* resolved_url);
    extern cef_resource_bundle_t*(*cef_resource_bundle_get_global)(void);
    extern cef_response_t*(*cef_response_create)(void);
    extern void(*cef_run_message_loop)(void);
    extern void(*cef_server_create)(const cef_string_t* address, uint16_t port, int backlog, struct _cef_server_handler_t* handler);
    extern void(*cef_set_crash_key_value)(const cef_string_t* key, const cef_string_t* value);
    extern void(*cef_set_osmodal_loop)(int osModalLoop);
    extern cef_shared_process_message_builder_t*(*cef_shared_process_message_builder_create)(const cef_string_t* name, size_t byte_size);
    extern void(*cef_shutdown)(void);
    extern cef_stream_reader_t*(*cef_stream_reader_create_for_data)(void* data, size_t size);
    extern cef_stream_reader_t*(*cef_stream_reader_create_for_file)(const cef_string_t* fileName);
    extern cef_stream_reader_t*(*cef_stream_reader_create_for_handler)(cef_read_handler_t* handler);
    extern cef_stream_writer_t*(*cef_stream_writer_create_for_file)(const cef_string_t* fileName);
    extern cef_stream_writer_t*(*cef_stream_writer_create_for_handler)(cef_write_handler_t* handler);
    extern int(*cef_string_ascii_to_utf16)(const char* src, size_t src_len, cef_string_utf16_t* output);
    extern int(*cef_string_ascii_to_wide)(const char* src, size_t src_len, cef_string_wide_t* output);
    extern cef_string_list_t(*cef_string_list_alloc)(void);
    extern void(*cef_string_list_append)(cef_string_list_t list, const cef_string_t* value);
    extern void(*cef_string_list_clear)(cef_string_list_t list);
    extern cef_string_list_t(*cef_string_list_copy)(cef_string_list_t list);
    extern void(*cef_string_list_free)(cef_string_list_t list);
    extern size_t(*cef_string_list_size)(cef_string_list_t list);
    extern int(*cef_string_list_value)(cef_string_list_t list, size_t index, cef_string_t* value);
    extern cef_string_map_t(*cef_string_map_alloc)(void);
    extern int(*cef_string_map_append)(cef_string_map_t map, const cef_string_t* key, const cef_string_t* value);
    extern void(*cef_string_map_clear)(cef_string_map_t map);
    extern int(*cef_string_map_find)(cef_string_map_t map, const cef_string_t* key, cef_string_t* value);
    extern void(*cef_string_map_free)(cef_string_map_t map);
    extern int(*cef_string_map_key)(cef_string_map_t map, size_t index, cef_string_t* key);
    extern size_t(*cef_string_map_size)(cef_string_map_t map);
    extern int(*cef_string_map_value)(cef_string_map_t map, size_t index, cef_string_t* value);
    extern cef_string_multimap_t(*cef_string_multimap_alloc)(void);
    extern int(*cef_string_multimap_append)(cef_string_multimap_t map, const cef_string_t* key, const cef_string_t* value);
    extern void(*cef_string_multimap_clear)(cef_string_multimap_t map);
    extern int(*cef_string_multimap_enumerate)(cef_string_multimap_t map, const cef_string_t* key, size_t value_index, cef_string_t* value);
    extern size_t(*cef_string_multimap_find_count)(cef_string_multimap_t map, const cef_string_t* key);
    extern void(*cef_string_multimap_free)(cef_string_multimap_t map);
    extern int(*cef_string_multimap_key)(cef_string_multimap_t map, size_t index, cef_string_t* key);
    extern size_t(*cef_string_multimap_size)(cef_string_multimap_t map);
    extern int(*cef_string_multimap_value)(cef_string_multimap_t map, size_t index, cef_string_t* value);
    extern cef_string_userfree_utf16_t(*cef_string_userfree_utf16_alloc)(void);
    extern void(*cef_string_userfree_utf16_free)(cef_string_userfree_utf16_t str);
    extern cef_string_userfree_utf8_t(*cef_string_userfree_utf8_alloc)(void);
    extern void(*cef_string_userfree_utf8_free)(cef_string_userfree_utf8_t str);
    extern cef_string_userfree_wide_t(*cef_string_userfree_wide_alloc)(void);
    extern void(*cef_string_userfree_wide_free)(cef_string_userfree_wide_t str);
    extern void(*cef_string_utf16_clear)(cef_string_utf16_t* str);
    extern int(*cef_string_utf16_cmp)(const cef_string_utf16_t* str1, const cef_string_utf16_t* str2);
    extern int(*cef_string_utf16_set)(const char16_t* src, size_t src_len, cef_string_utf16_t* output, int copy);
    extern int(*cef_string_utf16_to_lower)(const char16_t* src, size_t src_len, cef_string_utf16_t* output);
    extern int(*cef_string_utf16_to_upper)(const char16_t* src, size_t src_len, cef_string_utf16_t* output);
    extern int(*cef_string_utf16_to_utf8)(const char16_t* src, size_t src_len, cef_string_utf8_t* output);
    extern int(*cef_string_utf16_to_wide)(const char16_t* src, size_t src_len, cef_string_wide_t* output);
    extern void(*cef_string_utf8_clear)(cef_string_utf8_t* str);
    extern int(*cef_string_utf8_cmp)(const cef_string_utf8_t* str1, const cef_string_utf8_t* str2);
    extern int(*cef_string_utf8_set)(const char* src, size_t src_len, cef_string_utf8_t* output, int copy);
    extern int(*cef_string_utf8_to_utf16)(const char* src, size_t src_len, cef_string_utf16_t* output);
    extern int(*cef_string_utf8_to_wide)(const char* src, size_t src_len, cef_string_wide_t* output);
    extern void(*cef_string_wide_clear)(cef_string_wide_t* str);
    extern int(*cef_string_wide_cmp)(const cef_string_wide_t* str1, const cef_string_wide_t* str2);
    extern int(*cef_string_wide_set)(const wchar_t* src, size_t src_len, cef_string_wide_t* output, int copy);
    extern int(*cef_string_wide_to_utf16)(const wchar_t* src, size_t src_len, cef_string_utf16_t* output);
    extern int(*cef_string_wide_to_utf8)(const wchar_t* src, size_t src_len, cef_string_utf8_t* output);
    extern cef_task_runner_t*(*cef_task_runner_get_for_current_thread)(void);
    extern cef_task_runner_t*(*cef_task_runner_get_for_thread)(cef_thread_id_t threadId);
    extern cef_thread_t*(*cef_thread_create)(const cef_string_t* display_name, cef_thread_priority_t priority, cef_message_loop_type_t message_loop_type, int stoppable, cef_com_init_mode_t com_init_mode);
    extern int(*cef_time_delta)(const cef_time_t* cef_time1, const cef_time_t* cef_time2, long long* delta);
    extern int(*cef_time_from_basetime)(const cef_basetime_t from, cef_time_t* to);
    extern int(*cef_time_from_doublet)(double time, cef_time_t* cef_time);
    extern int(*cef_time_from_timet)(time_t time, cef_time_t* cef_time);
    extern int(*cef_time_now)(cef_time_t* cef_time);
    extern int(*cef_time_to_basetime)(const cef_time_t* from, cef_basetime_t* to);
    extern int(*cef_time_to_doublet)(const cef_time_t* cef_time, double* time);
    extern int(*cef_time_to_timet)(const cef_time_t* cef_time, time_t* time);
    extern void(*cef_trace_counter)(const char* category, const char* name, const char* value1_name, uint64_t value1_val, const char* value2_name, uint64_t value2_val);
    extern void(*cef_trace_counter_id)(const char* category, const char* name, uint64_t id, const char* value1_name, uint64_t value1_val, const char* value2_name, uint64_t value2_val);
    extern void(*cef_trace_event_async_begin)(const char* category, const char* name, uint64_t id, const char* arg1_name, uint64_t arg1_val, const char* arg2_name, uint64_t arg2_val);
    extern void(*cef_trace_event_async_end)(const char* category, const char* name, uint64_t id, const char* arg1_name, uint64_t arg1_val, const char* arg2_name, uint64_t arg2_val);
    extern void(*cef_trace_event_async_step_into)(const char* category, const char* name, uint64_t id, uint64_t step, const char* arg1_name, uint64_t arg1_val);
    extern void(*cef_trace_event_async_step_past)(const char* category, const char* name, uint64_t id, uint64_t step, const char* arg1_name, uint64_t arg1_val);
    extern void(*cef_trace_event_begin)(const char* category, const char* name, const char* arg1_name, uint64_t arg1_val, const char* arg2_name, uint64_t arg2_val);
    extern void(*cef_trace_event_end)(const char* category, const char* name, const char* arg1_name, uint64_t arg1_val, const char* arg2_name, uint64_t arg2_val);
    extern void(*cef_trace_event_instant)(const char* category, const char* name, const char* arg1_name, uint64_t arg1_val, const char* arg2_name, uint64_t arg2_val);
    extern cef_string_userfree_t(*cef_uridecode)(const cef_string_t* text, int convert_to_utf8, cef_uri_unescape_rule_t unescape_rule);
    extern cef_string_userfree_t(*cef_uriencode)(const cef_string_t* text, int use_plus);
    extern cef_urlrequest_t*(*cef_urlrequest_create)(struct _cef_request_t* request, struct _cef_urlrequest_client_t* client, struct _cef_request_context_t* request_context);
    extern cef_v8context_t*(*cef_v8context_get_current_context)(void);
    extern cef_v8context_t*(*cef_v8context_get_entered_context)(void);
    extern int(*cef_v8context_in_context)(void);
    extern cef_v8stack_trace_t*(*cef_v8stack_trace_get_current)(int frame_limit);
    extern cef_v8value_t*(*cef_v8value_create_array)(int length);
    extern cef_v8value_t*(*cef_v8value_create_array_buffer)(void* buffer, size_t length, cef_v8array_buffer_release_callback_t* release_callback);
    extern cef_v8value_t*(*cef_v8value_create_bool)(int value);
    extern cef_v8value_t*(*cef_v8value_create_date)(cef_basetime_t date);
    extern cef_v8value_t*(*cef_v8value_create_double)(double value);
    extern cef_v8value_t*(*cef_v8value_create_function)(const cef_string_t* name, cef_v8handler_t* handler);
    extern cef_v8value_t*(*cef_v8value_create_int)(int32_t value);
    extern cef_v8value_t*(*cef_v8value_create_null)(void);
    extern cef_v8value_t*(*cef_v8value_create_object)(cef_v8accessor_t* accessor, cef_v8interceptor_t* interceptor);
    extern cef_v8value_t*(*cef_v8value_create_promise)(void);
    extern cef_v8value_t*(*cef_v8value_create_string)(const cef_string_t* value);
    extern cef_v8value_t*(*cef_v8value_create_uint)(uint32_t value);
    extern cef_v8value_t*(*cef_v8value_create_undefined)(void);
    extern cef_value_t*(*cef_value_create)(void);
    extern cef_waitable_event_t*(*cef_waitable_event_create)(int automatic_reset, int initially_signaled);
    extern cef_string_userfree_t(*cef_write_json)(struct _cef_value_t* node, cef_json_writer_options_t options);
    extern cef_xml_reader_t*(*cef_xml_reader_create)(struct _cef_stream_reader_t* stream, cef_xml_encoding_type_t encodingType, const cef_string_t* URI);
    extern int(*cef_zip_directory)(const cef_string_t* src_dir, const cef_string_t* dest_file, int include_hidden_files);
    extern cef_zip_reader_t*(*cef_zip_reader_create)(struct _cef_stream_reader_t* stream);
}

bool loadCEFLibrary(const std::string& libPath);

#endif // CEF_LOADER_H
