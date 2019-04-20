#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dbus/dbus.h>

void sendsignal(char* sigvalue);
void receive(void);
void listen(void);
void query(char* param);

void sendsignal(char* sigvalue)
{
	DBusMessage *msg;
	DBusMessageIter args;
	DBusConnection *conn;
	DBusError err;
	dbus_uint32_t serial = 0;
	int ret;

	printf("Sending signal with value %s\n", sigvalue);

	dbus_error_init(&err);

	conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
	if(dbus_error_is_set(&err))
	{
		fprintf(stderr, "DBus connection error(%s)\n", err.message);
		dbus_error_free(&err);
	}
	if(conn == NULL)
	{
		fprintf(stderr, "DBus connection == NULL\n");
		exit(EXIT_FAILURE);
	}


	//02-Register name
	ret = dbus_bus_request_name(conn, "test.signal.source", DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
	if(dbus_error_is_set(&err))
	{
		fprintf(stderr, "DBus Name error(%s)\n", err.message);
		dbus_error_free(&err);
	}
	if(ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
	{
		fprintf(stderr, "ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER\n");
		exit(EXIT_FAILURE);	
	}

	//03-Create signal
	msg = dbus_message_new_signal("/test/signal/Object", //object name of signal
								  "test.signal.Type",    //interface name of signal
								  "Test");				 //name of signal
	if(msg == NULL)
	{
		fprintf(stderr, "Msg == NULL\n");
		exit(EXIT_FAILURE);
	}

	//04-Append argument to signal
	dbus_message_iter_init_append(msg, &args);

	if(!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &sigvalue))
	{
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	//05-Send the message and flush the connection
	if(!dbus_connection_send(conn, msg, &serial))
	{
		fprintf(stderr, "Out of Memory in send\n");
		exit(EXIT_FAILURE);
	}
	dbus_connection_flush(conn);

	printf("Signal sent\n");

	//06-free the message and close connection
	dbus_message_unref(msg);
	//dbus_connection_close(conn);
}

//listen for signals of the bus
void receive(void)
{
	DBusMessage* msg;
	DBusMessageIter args;
	DBusConnection* conn;
	DBusError err;
	int ret;
	char* sigvalue;

	printf("Listening for signals\n");

	//01-initialize errors
	dbus_error_init(&err);

	//02-connect to the bus and check errors
	conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
	if(dbus_error_is_set(&err))
	{
		fprintf(stderr, "ERROR: receive(): dbus_bus_get(): %s\n", err.message);
		exit(EXIT_FAILURE);
	}
	if(NULL == conn)
	{
		fprintf(stderr, "ERROR: conn == NULL\n");
		exit(EXIT_FAILURE);
	}

	//03-request our name on dbus and check for error
	ret = dbus_bus_request_name(conn, "test.signal.sink", DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
	if(dbus_error_is_set(&err))
	{
		fprintf(stderr, "ERROR: receive(): dbus_bus_request_name(): %s\n", err.message);
		exit(EXIT_FAILURE);
	}
	if(DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret)
	{
		fprintf(stderr, "ERROR: receive(): Not primary owner\n");
		exit(EXIT_FAILURE);
	}

	//04-add/register a rule for which we are expecting a signal to be received
	dbus_bus_add_match(conn, "type='signal', interface='test.signal.Type'", &err);
	if(dbus_error_is_set(&err))
	{
		fprintf(stderr, "ERROR: receive():dbus_bus_add_match(): %s\n", err.message);
		exit(EXIT_FAILURE);
	}
	printf("Match rule sent\n");

	//loop listening for signals emitted by others
	while(1)
	{
		//non-blocking read for next available message
		dbus_connection_read_write(conn, 0);
		msg = dbus_connection_pop_message(conn);
		//loop again if we havent signal message
		if(msg == NULL)
		{
			sleep(1);
			continue;
		}

		//check if the message is signal from correct interface and correct name
		if(dbus_message_is_signal(msg, "test.signal.Type", "Test"))
		{
			//read paramters
			if(!dbus_message_iter_init(msg, &args))
				fprintf(stderr, "ERROR: Message has no parameters\n");
			else if(DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args))
				fprintf(stderr, "ERROR: Argument is not string\n");
			else
				dbus_message_iter_get_basic(&args, &sigvalue);

			printf("Got signal with value %s\n", sigvalue);
		}

		//free message
		dbus_message_unref(msg);
	}

	//close connection
	dbus_connection_close(conn);
}

// void receive(void)
// {
// 	DBusMessage* msg;
// 	DBusMessageIter args;
// 	DBusConnection* conn;
// 	DBusError err;
// 	int ret;
// 	char* sigvalue;
// 	char array[2048];

// 	printf("Listening for signals\n");

// 	//01-initialize errors
// 	dbus_error_init(&err);

// 	//02-connect to the bus and check errors
// 	conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
// 	if(dbus_error_is_set(&err))
// 	{
// 		fprintf(stderr, "ERROR: receive(): dbus_bus_get(): %s\n", err.message);
// 		exit(EXIT_FAILURE);
// 	}
// 	if(NULL == conn)
// 	{
// 		fprintf(stderr, "ERROR: conn == NULL\n");
// 		exit(EXIT_FAILURE);
// 	}

// 	//03-request our name on dbus and check for error
// 	ret = dbus_bus_request_name(conn, "test.signal.sink", DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
// 	if(dbus_error_is_set(&err))
// 	{
// 		fprintf(stderr, "ERROR: receive(): dbus_bus_request_name(): %s\n", err.message);
// 		exit(EXIT_FAILURE);
// 	}
// 	if(DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret)
// 	{
// 		fprintf(stderr, "ERROR: receive(): Not primary owner\n");
// 		exit(EXIT_FAILURE);
// 	}

// 	//04-add/register a rule for which we are expecting a signal to be received
// 	//dbus_bus_add_match(conn, "type='signal', interface='test.signal.Type'", &err);
// 	dbus_bus_add_match(conn, "type='signal', interface='org.gtk.Actions'", &err);
// 	if(dbus_error_is_set(&err))
// 	{
// 		fprintf(stderr, "ERROR: receive():dbus_bus_add_match(): %s\n", err.message);
// 		exit(EXIT_FAILURE);
// 	}
// 	printf("Match rule sent\n");

// 	//loop listening for signals emitted by others
// 	while(1)
// 	{
// 		//non-blocking read for next available message
// 		dbus_connection_read_write(conn, 0);
// 		msg = dbus_connection_pop_message(conn);
// 		//loop again if we havent signal message
// 		if(msg == NULL)
// 		{
// 			sleep(1);
// 			continue;
// 		}

// 		//check if the message is signal from correct interface and correct name
// 		if(dbus_message_is_signal(msg, "org.gtk.Actions", "Changed"))
// 		{
// 			//read paramters
// 			if(!dbus_message_iter_init(msg, &args))
// 				fprintf(stderr, "ERROR: Message has no parameters\n");
// 			else if(DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&args))
// 				fprintf(stderr, "ERROR: Argument is not array\n");
// 			// else
// 			// 	dbus_message_iter_get_basic(&args, &array);

// 			//printf("Got signal with value %s\n", sigvalue);
// 		}

// 		//free message
// 		dbus_message_unref(msg);
// 	}

// 	//close connection
// 	dbus_connection_close(conn);
// }

void reply_to_method_call(DBusMessage *msg, DBusConnection *conn)
{
	DBusMessage* reply;
	DBusMessageIter args;
	dbus_bool_t stat = TRUE;
	dbus_uint32_t level = 21614;
	dbus_uint32_t serial = 0;
	char* param = "";

	//read message arguments
	if(!dbus_message_iter_init(msg, &args))
		fprintf(stderr, "ERROR: reply_to_method_call(): Message has no argument\n");
	else if(DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args))
		fprintf(stderr, "ERROR: reply_to_method_call(): Argument is not string\n");
	else
		dbus_message_iter_get_basic(&args, &param);

	printf("Method called with %s\n", param);

	//create reply from message
	reply = dbus_message_new_method_return(msg);

	//add arguments to reply
	dbus_message_iter_init_append(reply, &args);
	if(!dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &stat))
	{
		fprintf(stderr, "ERROR: reply_to_method_call(): dbus_message_iter_append_basic: Out of memory\n");
		exit(EXIT_FAILURE);
	}
	if(!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &level))
	{
		fprintf(stderr, "ERROR: reply_to_method_call(): dbus_message_iter_append_basic: Out of memory\n");
		exit(EXIT_FAILURE);
	}

	//send the reply and flush the connection
	if(!dbus_connection_send(conn, reply, &serial))
	{
		fprintf(stderr, "ERROR: reply_to_method_call(): dbus_connection_send() failed.\n");
		exit(EXIT_FAILURE);
	}
	dbus_connection_flush(conn);

	//free the reply message
	dbus_message_unref(reply);
}

//server that exposes a method call and waits for it to be called
void listen(void)
{
	DBusMessage* msg;
	DBusMessage* reply;
	DBusMessageIter args;
	DBusConnection* conn;
	DBusError err;
	int ret;
	char* param;

	printf("Listening for method calls\n");

	//01-initialize errors
	dbus_error_init(&err);

	//02-connect to the bus and check for errors
	conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
	if(dbus_error_is_set(&err))
	{
		fprintf(stderr, "ERROR:listen():dbus_bus_get():%s\n", err.message);
		exit(EXIT_FAILURE);
	}
	if(NULL == conn)
	{
		fprintf(stderr, "ERROR: conn == NULL\n");
		exit(EXIT_FAILURE);
	}

	//03-request our name on the bus and check for error
	ret = dbus_bus_request_name(conn, "test.method.server", DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
	if(dbus_error_is_set(&err))
	{
		fprintf(stderr, "ERROR:listen():dbus_bus_request_name():%s\n", err.message);
		exit(EXIT_FAILURE);
	}
	if(DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret)
	{
		fprintf(stderr, "ERROR:listen(): Not primary owner(%d)\n", ret);
		exit(EXIT_FAILURE);
	}

	//loop, testing for new message
	while(TRUE)
	{
		//non-blocking read of next available message
		dbus_connection_read_write(conn, 0);
		msg = dbus_connection_pop_message(conn);
		//loop again if we didn't receive message
		if(msg == NULL)
		{
			sleep(1);
			continue;
		}

		//check this if it is right interface and right method call
		if(dbus_message_is_method_call(msg, "test.method.Type", "Method"))
			reply_to_method_call(msg, conn);	//our own function, not DBUS api
		
		//free message
		dbus_message_unref(msg);
	}

	//close connection
	dbus_connection_close(conn);
}

// call a method on remote object
void query(char* param)
{
	DBusMessage *msg;
	DBusMessageIter args;
	DBusConnection *conn;
	DBusError err;
	DBusPendingCall *pending;
	int ret;
	dbus_bool_t stat;
	dbus_uint32_t level;

	//01-initialize errors
	dbus_error_init(&err);

	//02-get the session bus
	conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
	if(dbus_error_is_set(&err))
	{
		fprintf(stderr,"ERROR: query(): dbus_error_is_set(): %s\n", err.message);
		exit(EXIT_FAILURE);
	}
	if(conn == NULL)
	{
		fprintf(stderr, "ERROR: query(): conn == NULL");
		exit(EXIT_FAILURE);
	}

	//03-request our bus name of dbus
	ret = dbus_bus_request_name(conn, "test.method.caller", DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
	if(dbus_error_is_set(&err))
	{
		fprintf(stderr, "ERROR: query(): dbus_bus_request_name(): %s\n", err.message);
		exit(EXIT_FAILURE);
	}
	if(ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
	{
		fprintf(stderr, "ERROR: bus name requester is not owner. Exiting....\n");
		exit(EXIT_FAILURE);
	}

	//04-create a new method call and check for error
	msg = dbus_message_new_method_call("test.method.server",  //target for method call
									   "/test/method/Object", //object to call on
									   "test.method.Type",	  //interface to call on
									   "Method");			  //method name			
	if(msg == NULL)
	{
		fprintf(stderr, "ERROR: query(): dbus_message_new_method_call(): msg == NULL");
		exit(EXIT_FAILURE);
	}

	//05-append arguments
	dbus_message_iter_init_append(msg, &args);
	if(!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &param))
	{
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	//06-send message and get reply
	if(!dbus_connection_send_with_reply(conn, msg, &pending, -1)) //-1: defualt timeout
	{
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}
	if(NULL == pending)
	{
		fprintf(stderr, "Pending call == NULL\n");
		exit(EXIT_FAILURE);
	}
	dbus_connection_flush(conn);

	printf("Request send\n");

	//07-free message
	dbus_message_unref(msg);

	//08-block until we receive a reply
	dbus_pending_call_block(pending);

	//09-get the reply message
	msg = dbus_pending_call_steal_reply(pending);
	if(msg == NULL)
	{
		fprintf(stderr, "Reply msg = NULL\n");
		exit(EXIT_FAILURE);
	}

	//10-free the pending message handle
	dbus_pending_call_unref(pending);

	//11-read the message parameters
	if(!dbus_message_iter_init(msg, &args))
		fprintf(stderr, "Reply message has no arguments\n");
	else if(DBUS_TYPE_BOOLEAN != dbus_message_iter_get_arg_type(&args))
		fprintf(stderr, "Argument is not boolean\n");
	else
		dbus_message_iter_get_basic(&args, &stat);

	if(!dbus_message_iter_next(&args))
		fprintf(stderr, "Reply message has too few arguments\n");
	else if(!dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_UINT32)
		fprintf(stderr, "Argument is not int\n");
	else
		dbus_message_iter_get_basic(&args, &level);

	printf("Got Reply: %d %d\n", stat, level);

	//12-free reply message and close connection if required
	dbus_message_unref(msg);
	//dbus_connection_close(conn);

}


int main(int argc, char* argv[])
{
	char *param = "no param";

	if(argc < 2)
	{
		printf("Usage: %s [send|receive|listen|query] [<param>]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if(argc <= 3 && argv[2] != NULL)
		param = argv[2];

	if(strcmp(argv[1], "send") == 0)
		sendsignal(param);
	else if(strcmp(argv[1], "receive") == 0)
		receive();
	else if(strcmp(argv[1], "listen") == 0)
		listen();
	else if(strcmp(argv[1], "query") == 0)
		query(param);
	else
		printf("Usage: %s [send|receive|listen|query] [<param>]\n", argv[0]);

	exit(EXIT_SUCCESS);
}