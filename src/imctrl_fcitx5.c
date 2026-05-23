/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#include "vim.h"

#if defined(FEAT_IMCTRL_FCITX5)

#include <dbus/dbus.h>

static DBusConnection *fcitx5_conn = NULL;
static DBusMessage *fcitx5_msg_activate = NULL;
static DBusMessage *fcitx5_msg_deactivate = NULL;
static DBusMessage *fcitx5_msg_state = NULL;

static int fcitx5_open();
static DBusMessage *fcitx5_create_msg(const char *method);
static DBusMessage *fcitx5_send_msg(DBusMessage* request);

    static int
fcitx5_open()
{
    if (fcitx5_conn)
	return 1;

    DBusError err;
    DBusConnection *conn = NULL;

    dbus_error_init(&err);
    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err))
    {
	ch_log(NULL, "fcitx: failed to connect D-Bus: %s", err.message);
	dbus_error_free(&err);
	return 0;
    }
    if (!conn)
    {
	ch_log(NULL, "fcitx: null connection without errors");
	return 0;
    }

    // Prepare messages
    fcitx5_msg_activate = fcitx5_create_msg("Activate");
    fcitx5_msg_deactivate = fcitx5_create_msg("Deactivate");
    fcitx5_msg_state = fcitx5_create_msg("State");
    if (!fcitx5_msg_activate || !fcitx5_msg_deactivate || !fcitx5_msg_state)
    {
	fcitx5_close();
	return 0;
    }

    fcitx5_conn = conn;
    return 1;
}

    void
fcitx5_close()
{
    if (fcitx5_msg_activate)
    {
	dbus_message_unref(fcitx5_msg_activate);
	fcitx5_msg_activate = NULL;
    }
    if (fcitx5_msg_deactivate)
    {
	dbus_message_unref(fcitx5_msg_deactivate);
	fcitx5_msg_deactivate = NULL;
    }
    if (fcitx5_msg_state)
    {
	dbus_message_unref(fcitx5_msg_state);
	fcitx5_msg_state = NULL;
    }
    if (fcitx5_conn)
    {
	dbus_connection_unref(fcitx5_conn);
	fcitx5_conn = NULL;
    }
}

    static DBusMessage *
fcitx5_create_msg(const char *method)
{
    DBusMessage *msg;
    msg = dbus_message_new_method_call(
	    "org.fcitx.Fcitx5",
	    "/controller",
	    "org.fcitx.Fcitx.Controller1",
	    method);
    if (!msg)
	ch_log(NULL, "fcitx: failed to create \"%s\" message", method);
    return msg;
}

    static DBusMessage *
fcitx5_send_msg(DBusMessage* request)
{
    DBusError err;
    DBusMessage *reply = NULL;

    dbus_error_init(&err);

    reply = dbus_connection_send_with_reply_and_block(fcitx5_conn,
	    request, -1, &err);
    if (dbus_error_is_set(&err))
    {
	ch_log(NULL, "fcitx: failed to %s method: %s", dbus_message_get_member(request), err.message);
        dbus_error_free(&err);
	if (reply)
	    dbus_message_unref(reply);
	return NULL;
    }

    return reply;
}

    static int
fcitx5_get_state()
{
    if (!fcitx5_open())
	return -1;

    int state = -1;
    DBusError err;
    DBusMessage *reply;

    dbus_error_init(&err);

    reply = fcitx5_send_msg(fcitx5_msg_state);
    if (!reply)
	return -1;

    if (!dbus_message_get_args(reply, &err, DBUS_TYPE_INT32, &state,
		DBUS_TYPE_INVALID))
    {
	ch_log(NULL, "fcitx: failed to parse State response: %s", err.message);
        dbus_error_free(&err);
    }

    dbus_message_unref(reply);
    return state;
}

    static void
fcitx5_set_state(int active)
{
    if (!fcitx5_open())
	return;
    DBusMessage *reply;
    reply = fcitx5_send_msg(active ? fcitx5_msg_activate : fcitx5_msg_deactivate);
    if (!reply)
	return;
    dbus_message_unref(reply);
}

    void
f_fcitx5_status(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = fcitx5_get_state() > 1 ? 1 : 0;
}

    void
f_fcitx5_activate(typval_T *argvars, typval_T *rettv UNUSED)
{
    if (in_vim9script() && check_for_float_or_nr_arg(argvars, 0) == FAIL)
	return;

    if (argvars[0].v_type != VAR_NUMBER)
	return;

    varnumber_T active = tv_get_number_chk(&argvars[0], NULL);
    fcitx5_set_state(active);
}

#endif // FEAT_IMCTRL_FCITX5
