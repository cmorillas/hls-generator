// cef_function_redirects.h
// Redirect CEF C API function calls to our dynamic loader
// This header is force-included when compiling libcef_dll_wrapper

#ifndef CEF_FUNCTION_REDIRECTS_H
#define CEF_FUNCTION_REDIRECTS_H

// Include ALL CEF C API headers to get all type definitions
#include "include/capi/cef_accessibility_handler_capi.h"
#include "include/capi/cef_app_capi.h"
#include "include/capi/cef_audio_handler_capi.h"
#include "include/capi/cef_auth_callback_capi.h"
#include "include/capi/cef_base_capi.h"
#include "include/capi/cef_browser_capi.h"
#include "include/capi/cef_browser_process_handler_capi.h"
#include "include/capi/cef_callback_capi.h"
#include "include/capi/cef_client_capi.h"
#include "include/capi/cef_command_handler_capi.h"
#include "include/capi/cef_command_line_capi.h"
#include "include/capi/cef_context_menu_handler_capi.h"
#include "include/capi/cef_cookie_capi.h"
#include "include/capi/cef_crash_util_capi.h"
#include "include/capi/cef_win_capi.h"
#include "include/capi/cef_devtools_message_observer_capi.h"
#include "include/capi/cef_dialog_handler_capi.h"
#include "include/capi/cef_display_handler_capi.h"
#include "include/capi/cef_dom_capi.h"
#include "include/capi/cef_download_handler_capi.h"
#include "include/capi/cef_download_item_capi.h"
#include "include/capi/cef_drag_data_capi.h"
#include "include/capi/cef_drag_handler_capi.h"
#include "include/capi/cef_extension_capi.h"
#include "include/capi/cef_extension_handler_capi.h"
#include "include/capi/cef_file_util_capi.h"
#include "include/capi/cef_find_handler_capi.h"
#include "include/capi/cef_focus_handler_capi.h"
#include "include/capi/cef_frame_capi.h"
#include "include/capi/cef_frame_handler_capi.h"
#include "include/capi/cef_i18n_util_capi.h"
#include "include/capi/cef_image_capi.h"
#include "include/capi/cef_jsdialog_handler_capi.h"
#include "include/capi/cef_keyboard_handler_capi.h"
#include "include/capi/cef_life_span_handler_capi.h"
#include "include/capi/cef_load_handler_capi.h"
#include "include/capi/cef_media_router_capi.h"
#include "include/capi/cef_menu_model_capi.h"
#include "include/capi/cef_menu_model_delegate_capi.h"
#include "include/capi/cef_navigation_entry_capi.h"
#include "include/capi/cef_origin_whitelist_capi.h"
#include "include/capi/cef_parser_capi.h"
#include "include/capi/cef_path_util_capi.h"
#include "include/capi/cef_permission_handler_capi.h"
#include "include/capi/cef_preference_capi.h"
#include "include/capi/cef_print_handler_capi.h"
#include "include/capi/cef_print_settings_capi.h"
#include "include/capi/cef_process_message_capi.h"
#include "include/capi/cef_process_util_capi.h"
#include "include/capi/cef_registration_capi.h"
#include "include/capi/cef_render_handler_capi.h"
#include "include/capi/cef_render_process_handler_capi.h"
#include "include/capi/cef_request_capi.h"
#include "include/capi/cef_request_context_capi.h"
#include "include/capi/cef_request_context_handler_capi.h"
#include "include/capi/cef_request_handler_capi.h"
#include "include/capi/cef_resource_bundle_capi.h"
#include "include/capi/cef_resource_bundle_handler_capi.h"
#include "include/capi/cef_resource_handler_capi.h"
#include "include/capi/cef_resource_request_handler_capi.h"
#include "include/capi/cef_response_capi.h"
#include "include/capi/cef_response_filter_capi.h"
#include "include/capi/cef_scheme_capi.h"
#include "include/capi/cef_server_capi.h"
#include "include/capi/cef_shared_memory_region_capi.h"
#include "include/capi/cef_shared_process_message_builder_capi.h"
#include "include/capi/cef_ssl_info_capi.h"
#include "include/capi/cef_ssl_status_capi.h"
#include "include/capi/cef_stream_capi.h"
#include "include/capi/cef_string_visitor_capi.h"
#include "include/capi/cef_task_capi.h"
#include "include/capi/cef_thread_capi.h"
#include "include/capi/cef_trace_capi.h"
#include "include/capi/cef_unresponsive_process_callback_capi.h"
#include "include/capi/cef_urlrequest_capi.h"
#include "include/capi/cef_v8_capi.h"
#include "include/capi/cef_values_capi.h"
#include "include/capi/cef_waitable_event_capi.h"
#include "include/capi/cef_x509_certificate_capi.h"
#include "include/capi/cef_xml_reader_capi.h"
#include "include/capi/cef_zip_reader_capi.h"
#include "include/internal/cef_export.h"
#ifdef PLATFORM_LINUX
#include "include/internal/cef_linux.h"
#elif defined(PLATFORM_WINDOWS)
#include "include/internal/cef_win.h"
#endif
#include "include/internal/cef_logging_internal.h"
#include "include/internal/cef_ptr.h"
#include "include/internal/cef_string.h"
#include "include/internal/cef_string_list.h"
#include "include/internal/cef_string_map.h"
#include "include/internal/cef_string_multimap.h"
#include "include/internal/cef_string_types.h"
#include "include/internal/cef_string_wrappers.h"
#include "include/internal/cef_thread_internal.h"
#include "include/internal/cef_time.h"
#include "include/internal/cef_time_wrappers.h"
#include "include/internal/cef_trace_event_internal.h"
#include "include/internal/cef_types.h"
#include "include/internal/cef_types_color.h"
#include "include/internal/cef_types_content_settings.h"
#include "include/internal/cef_types_geometry.h"
#include "include/internal/cef_types_linux.h"
#include "include/internal/cef_types_runtime.h"
#include "include/internal/cef_types_wrappers.h"

// Now include our loader (which expects CEF types to be defined)
#include "cef_loader.h"

// Redirect all CEF C API functions to our loaded pointers
#define cef_add_cross_origin_whitelist_entry CEFLib::cef_add_cross_origin_whitelist_entry
#define cef_base64decode CEFLib::cef_base64decode
#define cef_base64encode CEFLib::cef_base64encode
#define cef_basetime_now CEFLib::cef_basetime_now
#define cef_begin_tracing CEFLib::cef_begin_tracing
#define cef_binary_value_create CEFLib::cef_binary_value_create
#define cef_browser_host_create_browser CEFLib::cef_browser_host_create_browser
#define cef_browser_host_create_browser_sync CEFLib::cef_browser_host_create_browser_sync
#define cef_clear_cross_origin_whitelist CEFLib::cef_clear_cross_origin_whitelist
#define cef_clear_scheme_handler_factories CEFLib::cef_clear_scheme_handler_factories
#define cef_command_line_create CEFLib::cef_command_line_create
#define cef_command_line_get_global CEFLib::cef_command_line_get_global
#define cef_cookie_manager_get_global_manager CEFLib::cef_cookie_manager_get_global_manager
#define cef_crash_reporting_enabled CEFLib::cef_crash_reporting_enabled
#define cef_create_context_shared CEFLib::cef_create_context_shared
#define cef_create_directory CEFLib::cef_create_directory
#define cef_create_new_temp_directory CEFLib::cef_create_new_temp_directory
#define cef_create_temp_directory_in_directory CEFLib::cef_create_temp_directory_in_directory
#define cef_create_url CEFLib::cef_create_url
#define cef_currently_on CEFLib::cef_currently_on
#define cef_delete_file CEFLib::cef_delete_file
#define cef_dictionary_value_create CEFLib::cef_dictionary_value_create
#define cef_directory_exists CEFLib::cef_directory_exists
#define cef_do_message_loop_work CEFLib::cef_do_message_loop_work
#define cef_drag_data_create CEFLib::cef_drag_data_create
#define cef_end_tracing CEFLib::cef_end_tracing
#define cef_execute_process CEFLib::cef_execute_process
#define cef_format_url_for_security_display CEFLib::cef_format_url_for_security_display
#define cef_get_current_platform_thread_handle CEFLib::cef_get_current_platform_thread_handle
#define cef_get_current_platform_thread_id CEFLib::cef_get_current_platform_thread_id
#define cef_get_exit_code CEFLib::cef_get_exit_code
#define cef_get_extensions_for_mime_type CEFLib::cef_get_extensions_for_mime_type
#define cef_get_mime_type CEFLib::cef_get_mime_type
#define cef_get_min_log_level CEFLib::cef_get_min_log_level
#define cef_get_path CEFLib::cef_get_path
#define cef_get_temp_directory CEFLib::cef_get_temp_directory
#define cef_get_vlog_level CEFLib::cef_get_vlog_level
#define cef_get_xdisplay CEFLib::cef_get_xdisplay
#define cef_image_create CEFLib::cef_image_create
#define cef_initialize CEFLib::cef_initialize
#define cef_is_cert_status_error CEFLib::cef_is_cert_status_error
#define cef_is_rtl CEFLib::cef_is_rtl
#define cef_launch_process CEFLib::cef_launch_process
#define cef_list_value_create CEFLib::cef_list_value_create
#define cef_load_crlsets_file CEFLib::cef_load_crlsets_file
#define cef_log CEFLib::cef_log
#define cef_media_router_get_global CEFLib::cef_media_router_get_global
#define cef_menu_model_create CEFLib::cef_menu_model_create
#define cef_now_from_system_trace_time CEFLib::cef_now_from_system_trace_time
#define cef_parse_json CEFLib::cef_parse_json
#define cef_parse_json_buffer CEFLib::cef_parse_json_buffer
#define cef_parse_jsonand_return_error CEFLib::cef_parse_jsonand_return_error
#define cef_parse_url CEFLib::cef_parse_url
#define cef_post_data_create CEFLib::cef_post_data_create
#define cef_post_data_element_create CEFLib::cef_post_data_element_create
#define cef_post_delayed_task CEFLib::cef_post_delayed_task
#define cef_post_task CEFLib::cef_post_task
#define cef_preference_manager_get_global CEFLib::cef_preference_manager_get_global
#define cef_print_settings_create CEFLib::cef_print_settings_create
#define cef_process_message_create CEFLib::cef_process_message_create
#define cef_quit_message_loop CEFLib::cef_quit_message_loop
#define cef_register_extension CEFLib::cef_register_extension
#define cef_register_scheme_handler_factory CEFLib::cef_register_scheme_handler_factory
#define cef_remove_cross_origin_whitelist_entry CEFLib::cef_remove_cross_origin_whitelist_entry
#define cef_request_context_create_context CEFLib::cef_request_context_create_context
#define cef_request_context_get_global_context CEFLib::cef_request_context_get_global_context
#define cef_request_create CEFLib::cef_request_create
#define cef_resolve_url CEFLib::cef_resolve_url
#define cef_resource_bundle_get_global CEFLib::cef_resource_bundle_get_global
#define cef_response_create CEFLib::cef_response_create
#define cef_run_message_loop CEFLib::cef_run_message_loop
#define cef_server_create CEFLib::cef_server_create
#define cef_set_crash_key_value CEFLib::cef_set_crash_key_value
#define cef_set_osmodal_loop CEFLib::cef_set_osmodal_loop
#define cef_shared_process_message_builder_create CEFLib::cef_shared_process_message_builder_create
#define cef_shutdown CEFLib::cef_shutdown
#define cef_stream_reader_create_for_data CEFLib::cef_stream_reader_create_for_data
#define cef_stream_reader_create_for_file CEFLib::cef_stream_reader_create_for_file
#define cef_stream_reader_create_for_handler CEFLib::cef_stream_reader_create_for_handler
#define cef_stream_writer_create_for_file CEFLib::cef_stream_writer_create_for_file
#define cef_stream_writer_create_for_handler CEFLib::cef_stream_writer_create_for_handler
#define cef_string_ascii_to_utf16 CEFLib::cef_string_ascii_to_utf16
#define cef_string_ascii_to_wide CEFLib::cef_string_ascii_to_wide
#define cef_string_list_alloc CEFLib::cef_string_list_alloc
#define cef_string_list_append CEFLib::cef_string_list_append
#define cef_string_list_clear CEFLib::cef_string_list_clear
#define cef_string_list_copy CEFLib::cef_string_list_copy
#define cef_string_list_free CEFLib::cef_string_list_free
#define cef_string_list_size CEFLib::cef_string_list_size
#define cef_string_list_value CEFLib::cef_string_list_value
#define cef_string_map_alloc CEFLib::cef_string_map_alloc
#define cef_string_map_append CEFLib::cef_string_map_append
#define cef_string_map_clear CEFLib::cef_string_map_clear
#define cef_string_map_find CEFLib::cef_string_map_find
#define cef_string_map_free CEFLib::cef_string_map_free
#define cef_string_map_key CEFLib::cef_string_map_key
#define cef_string_map_size CEFLib::cef_string_map_size
#define cef_string_map_value CEFLib::cef_string_map_value
#define cef_string_multimap_alloc CEFLib::cef_string_multimap_alloc
#define cef_string_multimap_append CEFLib::cef_string_multimap_append
#define cef_string_multimap_clear CEFLib::cef_string_multimap_clear
#define cef_string_multimap_enumerate CEFLib::cef_string_multimap_enumerate
#define cef_string_multimap_find_count CEFLib::cef_string_multimap_find_count
#define cef_string_multimap_free CEFLib::cef_string_multimap_free
#define cef_string_multimap_key CEFLib::cef_string_multimap_key
#define cef_string_multimap_size CEFLib::cef_string_multimap_size
#define cef_string_multimap_value CEFLib::cef_string_multimap_value
#define cef_string_userfree_utf16_alloc CEFLib::cef_string_userfree_utf16_alloc
#define cef_string_userfree_utf16_free CEFLib::cef_string_userfree_utf16_free
#define cef_string_userfree_utf8_alloc CEFLib::cef_string_userfree_utf8_alloc
#define cef_string_userfree_utf8_free CEFLib::cef_string_userfree_utf8_free
#define cef_string_userfree_wide_alloc CEFLib::cef_string_userfree_wide_alloc
#define cef_string_userfree_wide_free CEFLib::cef_string_userfree_wide_free
#define cef_string_utf16_clear CEFLib::cef_string_utf16_clear
#define cef_string_utf16_cmp CEFLib::cef_string_utf16_cmp
#define cef_string_utf16_set CEFLib::cef_string_utf16_set
#define cef_string_utf16_to_lower CEFLib::cef_string_utf16_to_lower
#define cef_string_utf16_to_upper CEFLib::cef_string_utf16_to_upper
#define cef_string_utf16_to_utf8 CEFLib::cef_string_utf16_to_utf8
#define cef_string_utf16_to_wide CEFLib::cef_string_utf16_to_wide
#define cef_string_utf8_clear CEFLib::cef_string_utf8_clear
#define cef_string_utf8_cmp CEFLib::cef_string_utf8_cmp
#define cef_string_utf8_set CEFLib::cef_string_utf8_set
#define cef_string_utf8_to_utf16 CEFLib::cef_string_utf8_to_utf16
#define cef_string_utf8_to_wide CEFLib::cef_string_utf8_to_wide
#define cef_string_wide_clear CEFLib::cef_string_wide_clear
#define cef_string_wide_cmp CEFLib::cef_string_wide_cmp
#define cef_string_wide_set CEFLib::cef_string_wide_set
#define cef_string_wide_to_utf16 CEFLib::cef_string_wide_to_utf16
#define cef_string_wide_to_utf8 CEFLib::cef_string_wide_to_utf8
#define cef_task_runner_get_for_current_thread CEFLib::cef_task_runner_get_for_current_thread
#define cef_task_runner_get_for_thread CEFLib::cef_task_runner_get_for_thread
#define cef_thread_create CEFLib::cef_thread_create
#define cef_time_delta CEFLib::cef_time_delta
#define cef_time_from_basetime CEFLib::cef_time_from_basetime
#define cef_time_from_doublet CEFLib::cef_time_from_doublet
#define cef_time_from_timet CEFLib::cef_time_from_timet
#define cef_time_now CEFLib::cef_time_now
#define cef_time_to_basetime CEFLib::cef_time_to_basetime
#define cef_time_to_doublet CEFLib::cef_time_to_doublet
#define cef_time_to_timet CEFLib::cef_time_to_timet
#define cef_trace_counter CEFLib::cef_trace_counter
#define cef_trace_counter_id CEFLib::cef_trace_counter_id
#define cef_trace_event_async_begin CEFLib::cef_trace_event_async_begin
#define cef_trace_event_async_end CEFLib::cef_trace_event_async_end
#define cef_trace_event_async_step_into CEFLib::cef_trace_event_async_step_into
#define cef_trace_event_async_step_past CEFLib::cef_trace_event_async_step_past
#define cef_trace_event_begin CEFLib::cef_trace_event_begin
#define cef_trace_event_end CEFLib::cef_trace_event_end
#define cef_trace_event_instant CEFLib::cef_trace_event_instant
#define cef_uridecode CEFLib::cef_uridecode
#define cef_uriencode CEFLib::cef_uriencode
#define cef_urlrequest_create CEFLib::cef_urlrequest_create
#define cef_v8context_get_current_context CEFLib::cef_v8context_get_current_context
#define cef_v8context_get_entered_context CEFLib::cef_v8context_get_entered_context
#define cef_v8context_in_context CEFLib::cef_v8context_in_context
#define cef_v8stack_trace_get_current CEFLib::cef_v8stack_trace_get_current
#define cef_v8value_create_array CEFLib::cef_v8value_create_array
#define cef_v8value_create_array_buffer CEFLib::cef_v8value_create_array_buffer
#define cef_v8value_create_bool CEFLib::cef_v8value_create_bool
#define cef_v8value_create_date CEFLib::cef_v8value_create_date
#define cef_v8value_create_double CEFLib::cef_v8value_create_double
#define cef_v8value_create_function CEFLib::cef_v8value_create_function
#define cef_v8value_create_int CEFLib::cef_v8value_create_int
#define cef_v8value_create_null CEFLib::cef_v8value_create_null
#define cef_v8value_create_object CEFLib::cef_v8value_create_object
#define cef_v8value_create_promise CEFLib::cef_v8value_create_promise
#define cef_v8value_create_string CEFLib::cef_v8value_create_string
#define cef_v8value_create_uint CEFLib::cef_v8value_create_uint
#define cef_v8value_create_undefined CEFLib::cef_v8value_create_undefined
#define cef_value_create CEFLib::cef_value_create
#define cef_waitable_event_create CEFLib::cef_waitable_event_create
#define cef_write_json CEFLib::cef_write_json
#define cef_xml_reader_create CEFLib::cef_xml_reader_create
#define cef_zip_directory CEFLib::cef_zip_directory
#define cef_zip_reader_create CEFLib::cef_zip_reader_create

#endif // CEF_FUNCTION_REDIRECTS_H
