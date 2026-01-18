#pragma once
#include <iostream>

// maybe do an automated-generation process here
enum RpcEnum : std::uint16_t
{
	rpc_connect,
	rpc_debug,

	rpc_server_tick,
	rpc_server_error_respond,
	rpc_server_register,
	rpc_server_log_in,
	rpc_server_set_name,
	rpc_server_set_language,
	rpc_server_send_text,
	rpc_server_print_user,
	rpc_server_print_room,
	rpc_server_goto_room,
	rpc_server_create_room,

	rpc_client_tick,
	rpc_client_error_respond,
	//rpc_client_register,
	rpc_client_log_in,
	rpc_client_set_name,
	rpc_client_set_language,
	rpc_client_send_text,
	rpc_client_print_user,
	rpc_client_print_room,
	rpc_client_goto_room,
	rpc_client_create_room,

	INVALID,
};