/*
 * GeeXboX Enna Media Center.
 * Copyright (C) 2005-2010 The Enna Project
 *
 * This file is part of Enna.
 *
 * Enna is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * Enna is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Enna; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*! \file
 * \brief Wiimote input module.
 *
 * This module supplies support for the Nintendo Wiimote controller.
 * Currently only the wiimote's input is sent to enna and thus peripherals such
 * the nunchuk, classic controller etc do not work yet.
 *
 *
 */

#include <bluetooth/bluetooth.h>
#include <cwiid.h>
#include <time.h>

#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "logs.h"
#include "input.h"
#include "module.h"
#include "buffer.h"

#include "view_list2.h"
#include "input.h"

/*!
 * \brief Define for the name of the module.
 */
#define ENNA_MODULE_NAME "input_wiimote"

/*!
 * \brief Maximum length of a Bluetooth address string.
 */
#define BA_STR_LEN 18

/*!
 * \brief Configuration data of one wiimote controller.
 *
 *  This structure holds information about a wiimote controller.
 */
typedef struct _Wiimote_Cfg
{
    char *name;                 /*!< Name of a wiimote. */
    int id;                     /*!< Numeric ID of a wiimote. */
    Eina_Bool connected;        /*!< Connection status indicator. */
    char bdaddr[BA_STR_LEN];    /*!< Bluetooth address string of a wiimote. */
    struct wiimote *wiimote;    /*!< Data structure of a wiimote. */
    Elm_Genlist_Item *item;     /*!< GUI element in the configuration panel. */
} Wiimote_Cfg;

/*!
 * \brief Data structure for the enna module.
 *
 * This data structure contains module specific data.
 */
typedef struct _Enna_Module_Wiimote
{
    Enna_Module *em;            /*!< Module list entry */
    int default_wiimote;        /*!< Default wiimote to be used as primary
                                  input control. */
    Eina_List *wiimotes;        /*!< List containing all available wiimotes. */
    Elm_Genlist_Item *scantop;  /*!< GUI element pointer to indicate the top
                                  of the scan results. */
    Elm_Genlist_Item *scanbot;  /*!< GUI element pointer to indicate the bottom
                                  of the scan results. */
    Elm_Genlist_Item *result;   /*!< GUI element pointer to indicate where scan
                                  results should start. */
} Enna_Module_Wiimote;

/* FIXME: Maybe move these variables inside the Enna_Module structure? */
static Enna_Module_Wiimote *mod = NULL;     /*!< The enna module.  */
static Enna_Config_Panel *_config_panel;    /*!< Configuration panel. */
static Evas_Object *_o_cfg_panel = NULL;    /*!< Configuration panel. */

/*!
 * \brief Data structure containing wiimote key mapping.
 *
 * This data structure maps keys received from the wiimote to certain
 * enna_input events. Also contained are the names for the received keys
 * in text format which can be used for debugging or GUI configuration.
 * At the moment the mapping is hard-coded.
 *
 */
static const struct {
    const char *keyname;    /*!< Name of a received key. */
    uint16_t button;        /*!< Received button keycode. */
    enna_input input;       /*!< Mapped input signal. */
} enna_wiimotemap[] = {
    { /* Wiimote */
        .keyname = "Wiimote Home",
        .button = CWIID_BTN_HOME,
        .input = ENNA_INPUT_HOME
    }, {
        .keyname = "Wiimote 1",
        .button = CWIID_BTN_1,
        .input = ENNA_INPUT_INFO,
    }, {
        .keyname = "Wiimote 2",
        .button = CWIID_BTN_1,
        .input = ENNA_INPUT_FULLSCREEN,
    }, {
        .keyname = "Wiimote A",
        .button = CWIID_BTN_A,
        .input = ENNA_INPUT_OK
    }, {
        .keyname = "Wiimote B",
        .button = CWIID_BTN_B,
        .input = ENNA_INPUT_BACK
    }, {
        .keyname = "Wiimote +",
        .button = CWIID_BTN_PLUS,
        .input = ENNA_INPUT_VOLPLUS
    }, {
        .keyname = "Wiimote -",
        .button = CWIID_BTN_MINUS,
        .input = ENNA_INPUT_VOLMINUS
    }, {
        .keyname = "Wiimote Left",
        .button = CWIID_BTN_LEFT,
        .input = ENNA_INPUT_LEFT
    }, {
        .keyname = "Wiimote Right",
        .button = CWIID_BTN_RIGHT,
        .input = ENNA_INPUT_RIGHT
    }, {
        .keyname = "Wiimote Up",
        .button = CWIID_BTN_UP,
        .input = ENNA_INPUT_UP
    }, {
        .keyname = "Wiimote Down",
        .button = CWIID_BTN_DOWN,
        .input = ENNA_INPUT_DOWN
    }, { /* The End */
        .keyname = NULL,
        .button = 0,
        .input = ENNA_INPUT_UNKNOWN
    }
};

/*****************************************************************************/
/*                          Public Module API                                */
/*****************************************************************************/


/*****************************************************************************/
/*                       Private Module Functions                            */
/*****************************************************************************/

/*!
 * \brief Sort Wiimote_Cfg based on contained id.
 *
 * This callback function is to be used to sort the Wiimote_Cfg based on
 * the id contained inside the struct.
 *
 * \pre None
 *
 * \post None
 *
 * \param[in] d1 a Wiimote_Cfg data pointer.
 * \param[in] d2 a Wiimote_Cfg data pointer.
 *
 * \return If d1 is equal to d2 0 is returned.
 * If d1 is greater than d2 -1 is returned.
 * If d1 is smaller than d2 1 is returned.
 * If d1 or d2 is NULL then the other is considered larger.
 */
static int
wiimotes_sort_cb(const void *d1, const void *d2)
{
    int id1, id2; /*!< The id's to be compared. */
    int retval;   /*!< Return value. */


    if (!d1)
    {
        retval = -1;
    }
    else if (!d2)
    {
        retval = 1;
    }
    else
    {
        id1 = ((Wiimote_Cfg *)d1)->id;
        id2 = ((Wiimote_Cfg *)d2)->id;
        retval = (id1 == id2) ? 0 : (id1 > id2) ? -1: 1;
    }

    return retval;
}

/*!
 * \brief Find a bluetooth address string in the wiimotes list.
 *
 * This function tries to find the supplied address string in the stored list
 * of bluetooth addresses.
 *
 *
 * \pre The input parameters are not checked if they are valid addresses.
 *
 * \post None
 *
 * \param[in] ba_str Bluetooth address string in the form of xx:xx:xx:xx:xx
 * where x is a hexadecimal between 0 and f.
 *
 * \return If the bluetooth address is found in the list 1 is returned,
 * otherwise 0.
 */
static int
wiimote_in_list(const char *ba_str)
{
    int retval;     /*!< Return value */
    Eina_List *l;   /*!< Internal list for EINA_LIST_FOREACH. */
    Wiimote_Cfg *w; /*!< Points to wiimote structure in list. */


    retval = 0;

    EINA_LIST_FOREACH(mod->wiimotes, l, w)
    {
        if (strncmp((const char *)&w->bdaddr, ba_str, BA_STR_LEN) == 0)
        {
            retval = 1;
            break;
        }
    }
    return retval;
}

/*!
 * \brief Maps a wiimote button press to an enna_input event.
 *
 * This function checks whether the received button variable contains
 * one of the keys in the button mapping table and then emits the corresponding
 * input event to enna.
 *
 * \pre None
 *
 * \post None
 *
 * \remark Currently only single presses can be handled so a key combination
 * of, say A+B is sent to enna as to seperate events, event A and event B.
 *
 * \param[in] mesg A cwiid button message.
 */
static void
wiimote_button_handler(struct cwiid_btn_mesg *mesg)
{
    int i;  /*< Iterator to go over the list. */


    for (i = 0; enna_wiimotemap[i].keyname != NULL; i++)
    {
        if (mesg->buttons & enna_wiimotemap[i].button)
        {
            enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME, "Key pressed: %s",
                    enna_wiimotemap[i].keyname);
            enna_input_event_emit(enna_wiimotemap[i].input);
        }
    }
}

/*!
 * \brief Maps a wiimote ir event to an enna_input event.
 *
 * This function goes through the list of received coordinates from the
 * infrared camera and averages a nearby group of coordinates. This then should
 * result in two set of coordinates. These two coordinates are then processed
 * to map to an x, y and rotation coordinate. Finally these coordinates are
 * converted to a cursor position and sent to enna.
 *
 * \pre There needs to be atleast two coordinates far enough appart from each
 * other to be identified as two dots.
 *
 * \post None
 *
 * \remark Currently only single presses can be handled so a key combination
 * of, say A+B is sent to enna as to seperate events, event A and event B.
 *
 * \param[in] mesg A cwiid button message.
 */
static void
wiimote_ir_handler(struct cwiid_ir_mesg *mesg)
{
    int i;  /*< Iterator to go over the list. */


    for (i = 0; i < CWIID_IR_SRC_COUNT; i++)
    {
        if (mesg->src[i].valid)
        {
            int size;   /*< Size of a received pixel. */


            if (mesg->src[i].size == -1)
            {
                size = 3;
            }
            else
            {
                size = mesg->src[i].size +1;
            }
            enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME, "%d, %d",
                    mesg->src[i].pos[CWIID_X], /* / CWIID_IR_X_MAX, */
                    mesg->src[i].pos[CWIID_Y] /* / CWIID_IR_Y_MAX */
                    );

        }
    }
}

/*!
 * \brief Callback to handle cwiid messages from a wiimote.
 *
 * Whenever a message is sent by the wiimote, be this a requested message or
 * based on an event, such as a keypress or movement, this function gets called
 * by cwiid.
 *
 * \pre None
 *
 * \post None
 *
 * \param[in] wiimote Pointer to cwiid wiimote structure.
 * \param[in] mesg_count number of received messages.
 * \param[in] mesg_array Pointer to message array union.
 * \param[in] timestamp Timestamp of received message.
 *
 * \remark It should be noted that the parameters are supplied by cwiid and
 * can not be changed.
 *
 */
static void
wiimote_mesg_cb(cwiid_wiimote_t *wiimote, int mesg_count,
                union cwiid_mesg mesg_array[], struct timespec *timestamp)
{
    int i;  /*< Iterator to go over the list. */


    for (i = 0; i < mesg_count; i++)
    {
        switch (mesg_array[i].type)
        {
        case CWIID_MESG_BTN:
            wiimote_button_handler(&mesg_array[i].btn_mesg);
            break;

        case CWIID_MESG_IR:
            wiimote_ir_handler(&mesg_array[i].ir_mesg);
            break;


        case CWIID_MESG_ERROR:
            /* TODO: somehow setup some error handling? Can't just
             * disconnect as this function is can be called via any thread.
             * Pass the wiimote pointer to a function that locks the List,
             * scans through it until it finds the wiimote, updates it's
             * error display maybe and then d/c it?
             */
            break;

        default:
            break;
        }
    }
}

/*!
 * \brief Free Wiimote_Cfg.
 *
 * This function closes a cwiid wiimote in a wiimote structure and free's the
 * the structure includeing the name.
 *
 *
 * \pre The item member will not be freed.
 *
 * \post None
 *
 * \param[in, out] w Wiimote_Cfg structure to be free'd. Will be NULL on
 * completion.
 *
 * \return
 */
static void
wiimote_free(Wiimote_Cfg *w)
{
    if (w)
    {
        ENNA_FREE(w->name);
        if (w->wiimote)
        {
            cwiid_close(w->wiimote);
        }
        ENNA_FREE(w);
    }
}

/*!
 * \brief Disconnects the referenced by wiimote id.
 *
 * This function finds the supplied id in the list of wiimotes, closes its
 * socket and does not remove it from the wiimotes structure.
 *
 * \pre The supplied ID should be in the list and must be unique in the list.
 * If it is not unique, the first occurrence of the ID will be closed.
 *
 * \post None
 *
 * \param[in] id integer referencing the wiimote in the list.
 *
 * \return If the wiimote is in the list, returns cwiid's close return value
 * being, 0 on success, errno otherwise. If the wiimote is not in the list
 * returns -1.
 */
static int
wiimote_close(int id)
{
    int retval;     /*!< Return value. */
    Eina_List *l;   /*!< Internal list for EINA_LIST_FOREACH. */
    Wiimote_Cfg *w; /*!< Points to wiimote structure in list. */


    retval = -1;

    EINA_LIST_FOREACH(mod->wiimotes, l, w)
    {
        if (w->id == id)
        {
            /* FIXME: Check cwiid_close return value, assumed 0 on error */
            retval = cwiid_close(w->wiimote);
            w->connected = (retval) ? EINA_TRUE : EINA_FALSE;
            w->wiimote = NULL;
            enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
                    "[%s] disconnected.", w->bdaddr);
            break;
        }
    }

    return retval;
}

/*!
 * \brief Connects to the wiimote referenced by id.
 *
 * This function finds the supplied id in the list of wiimotes, opens its
 * socket and does not add it to the wiimotes structure.
 *
 * \pre The supplied ID should be in the list and must be unique in the list.
 * If it is not unique, the first occurrence of the ID will be opened.
 *
 * \pre None
 *
 * \post None
 *
 * \param[in] id integer referencing the wiimote in the list.
 *
 * \return 0 on failure and -1 otherwise.
 */
static int
wiimote_open(int id)
{
    int retval;     /*!< Return value. */
    Eina_List *l;   /*!< Internal list for EINA_LIST_FOREACH. */
    Wiimote_Cfg *w; /*!< Points to wiimote structure in list. */


    retval = -1;

    EINA_LIST_FOREACH(mod->wiimotes, l, w)
    {
        if (w->id == id)
        {
            bdaddr_t bdaddr;        /*!< Bluetooth address */
            str2ba(w->bdaddr, &bdaddr);
            w->wiimote = cwiid_open(&bdaddr, CWIID_FLAG_MESG_IFC);
            if (w->wiimote != NULL)
            {
                w->connected = EINA_TRUE;
                if (cwiid_set_mesg_callback(w->wiimote, &wiimote_mesg_cb))
                {
                    wiimote_close(id);
                }
                enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
                        "Connected with wiimote [%s]", w->bdaddr);
                cwiid_set_rpt_mode(w->wiimote,
                        CWIID_RPT_STATUS | CWIID_RPT_BTN);
                cwiid_set_led(w->wiimote, w->id);
                retval = 0;
            }
            else
            {
                enna_log(ENNA_MSG_WARNING, ENNA_MODULE_NAME,
                        "Failed to connect wiimote [%s]", w->bdaddr);
                w->connected = EINA_FALSE;
            }
            break;
        }
    }

    return retval;
}

/*!
 * \brief Removes a wiimote from the wiimotes list referenced by id.
 *
 * This function finds the supplied id in the list of wiimotes, closes it
 * if it is still connected and removes it from the list.
 *
 * \pre The supplied ID should be in the list and must be unique in the list.
 * If it is not unique, the first occurrence of the ID will be removed.
 *
 * \post None
 *
 * \param[in] id integer referencing the wiimote in the list.
 *
 * \return If the wiimote is still connected, returns cwiid's close return
 * value, being 0 on success, errno otherwise. If the wiimote is not connected
 * returns 0 on failure and -1 otherwise.
 */
static int
wiimote_remove(int id)
{
    int retval;     /*!< Return value. */
    Eina_List *l;   /*!< Internal list for EINA_LIST_FOREACH. */
    Wiimote_Cfg *w; /*!< Points to wiimote structure in list. */

    retval = -1;

    EINA_LIST_FOREACH(mod->wiimotes, l, w)
    {
        if (w->id == id)
        {
            if (w->connected)
            {
                retval = wiimote_close(w->id);
            }
            mod->wiimotes = eina_list_remove(mod->wiimotes, w);
            enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
                    "[%s] with id: %d removed from list.", w->bdaddr, w->id);
            wiimote_free(w);
            retval = 0;
            break;
        }
    }

    return retval;
}

/*!
 * \brief Adds a wiimote to the list of wiimotes.
 *
 * This function adds a wiimote to the list of wiimotes. If the supplied ID is
 * less then 1, the wiimote will be assigned the first available slot.
 * Otherwise the supplied ID will be assigned if available. If it is not
 * available the first available slot will be assigned.
 *
 * \pre None
 *
 * \post None
 *
 * \param[in] id integer referencing the wiimote in the list.
 * \param[in] name A string to be used for the wiimote's name.
 * \param[in] bdaddr Blutooth address string.
 *
 * \return NULL on failure, wiimote structure on success.
 */
Wiimote_Cfg *
wiimote_add(int id, const char *name, const char *bdaddr)
{
    Eina_List *l;   /*!< Internal list for EINA_LIST_FOREACH. */
    Wiimote_Cfg *w; /*!< Points to wiimote structure in list. */


    w = NULL;

    if (id < 1)
    {
        id = 1;
    }

    EINA_LIST_FOREACH(mod->wiimotes, l, w)
    {
        if (w->id == id)
        {
            /* id all ready existed, try a new id and reset.
             * Potential bug if there would be more than sizeof(int) wiimotes.
             */
            id++;
            l = mod->wiimotes;
            w = eina_list_data_get(l);
        }
    }

    w = ENNA_NEW(Wiimote_Cfg, 1);
    w->id = id;
    w->connected = EINA_FALSE;
    if (name)
    {
        w->name = strdup(name);
    }
    else
    {
        w->name = strdup(_("Unnamed Wiimote"));
    }
    if (bachk(bdaddr))
    {
        strcpy(w->bdaddr, "00:00:00:00:00");
    }
    else
    {
        strcpy(w->bdaddr, bdaddr);
    }

    mod->wiimotes = eina_list_sorted_insert(mod->wiimotes, wiimotes_sort_cb, w);

    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
           "Successfully added [%s] with id: %d to list.", w->bdaddr, id);
    return w;
}

/*!
 * \brief Retrieve battery level from wiimote.
 *
 * This function will retrieve battery infromation from the wiimote and
 * translates it into a percentage.
 *
 * \pre None
 *
 * \post None
 *
 * \param[in] w Wiimote configuration structure.
 *
 * \return the wiimote battery percentage between 0 and 100%. On error -1.
 */
static int
wiimote_get_battery_level(Wiimote_Cfg *w)
{
    int battery_level;  /*!< Battery level in percent. */


    battery_level = 100;

    return (battery_level > 100) ? -1 :
        (battery_level < 0 ? -1: battery_level);
}


/****************************************************************************/
/*                          Callback Functions                              */
/****************************************************************************/

/*!
 * \brief Connect button callback.
 *
 * This function serves as a callback for the connect button on the
 * configuration panel. If the wiimote is connected, the wiimote gets
 * disconnected, otherwise try to connect it.
 *
 * \pre None
 *
 * \post None
 *
 * \param[in, out] data Pointer to the Wiimote_Cfg structure.
 * \param[in, out] widget Pointer to the calling widget.
 *
 * \remark FIXME Currently this function is called by a button. It should be
 * called by a toggle switch, however this is not yet implemented in enna.
 */
static void
_wiimote_config_panel_connect_cb(void *data, Enna_View_List2_Widget *widget)
{
    Wiimote_Cfg *w; /*!< Points to wiimote structure in list. */


    w = data;

    if (w->connected)
    {
        wiimote_close(w->id);
    }
    else
    {
        wiimote_open(w->id);
    }
}

/*!
 * \brief Editable text label callback.
 *
 * This function serves as a callback for when the name of a wiimote is changed
 * in the GUI.
 *
 * \pre None
 *
 * \post None
 *
 * \param[in, out] data Not used.
 * \param[in, out] widget Pointer to the calling widget.
 *
 * \remark This function doesn't appear to be called yet by enna.
 */
static void
_wiimote_config_panel_rename_cb(void *data, Enna_View_List2_Widget *widget)
{
}

/*!
 * \brief Remove button callback.
 *
 * This function serves as a callback for the remove button on the
 * configuration panel. Also removes the display entry from the list.
 *
 * \pre None
 *
 * \post None
 *
 * \param[in] data Pointer to Wiimote_Cfg structure containing wiimote.
 * \param[in, out] widget Pointer to the calling widget.
 *
 * \remark
 */
static void
_wiimote_config_panel_remove_cb(void *data, Enna_View_List2_Widget *widget)
{
    Wiimote_Cfg *w; /*!< Points to wiimote structure in list. */


    w = data;

    wiimote_remove(w->id);
    enna_list2_item_del(w->item);
}

/*!
 * \brief Creates a wiimote GUI view_list entry.
 *
 * This function creates a GUI entry for the supplied wiimote at the supplied
 * position in the configuration panel.
 *
 * \pre None
 *
 * \post None
 *
 * \param[in] position The position after which the GUI item will be created.
 * \param[in] w Pointer to the Wiimote_Cfg structure.
 *
 * \return Pointer to the created GUI entry.
 */
Elm_Genlist_Item *
_wiimote_config_panel_wiimote_entry_create(Elm_Genlist_Item *position,
                                           Wiimote_Cfg *w)
{
    Elm_Genlist_Item *item; /*!< GUI element that holds a list entry. */
    char wiimote_str[256];  /*!< Various wiimote strings. */
    char battery_level[5];  /*!< Battery indicator string, such as
                                 100%\\0 for examle. */


    snprintf(wiimote_str, sizeof(wiimote_str), "%d.", w->id);

    item = enna_list2_item_insert_after(_o_cfg_panel, position,
            wiimote_str, _("Device that are in memory."),
            NULL, NULL, NULL);
    w->item = item;

    snprintf(wiimote_str, sizeof(wiimote_str), "[%s]", w->bdaddr);
    enna_list2_item_entry_add(item, NULL, wiimote_str,
            NULL, NULL);

    /* TODO Add battery indicator, atm we only have text, so we use that.
     * it's ugly and should be changed to something else.
     * TODO Also keeping it up to date needs to be done.
     */
    if (w->connected)
    {
        sprintf(battery_level, "%02d%%", wiimote_get_battery_level(w));
    }
    else
    {
        strcpy(battery_level, _("N/A"));
    }
    enna_list2_item_entry_add(item, NULL, battery_level, NULL, NULL);
    /* insert spacer here */
    enna_list2_item_entry_add(item, NULL, w->name,
            _wiimote_config_panel_rename_cb, w);
    /* FIXME: temporarly use button instead of toggle */
    enna_list2_item_button_add(item, NULL, _("Connect"),
            _wiimote_config_panel_connect_cb, w);
    /*
    enna_list2_item_toggle_add(item, NULL, _("Connected"),
            _("Not connected"), _wiimote_config_panel_connect_cb, w);
    */
    enna_list2_item_button_add(item, NULL, _("Remove"),
            _wiimote_config_panel_remove_cb, w);

    return item;
}

/*!
 * \brief Add button callback.
 *
 * This function serves as a callback for the add button on the configuration
 * panel. The wiimote only gets added if its bluetooth address string isn't
 * already in the list. After adding it, we will try to connect to it and then
 * create an entry on the configuration panel.
 *
 * \pre None
 *
 * \post None
 *
 * \param[in, out] data Pointer to Wiimote_Cfg structure.
 * \param[in, out] widget Pointer to the calling widget.
 *
 * \remark The supplied Wiimote_Cfg structure is only used as a dummy to supply
 * wiimote_add with data, and thus id, name and bdaddr can only and should only
 * be used.
 *
 * \return Pointer to a newly added Wiimote_Cfg structure.
 */
static void
_wiimote_config_panel_add_cb(void *data, Enna_View_List2_Widget *widget)
{
    Wiimote_Cfg *w; /*!< Points to wiimote structure in list */


    w = data;

    enna_list2_item_del(w->item);
    if (!wiimote_in_list(w->bdaddr))
    {
        w = wiimote_add(w->id, w->name, w->bdaddr);
        wiimote_open(w->id);
        _wiimote_config_panel_wiimote_entry_create(mod->result, w);
    }
}

/* Delete items starting at from ending at to, not including to.
 * Parameter *from: First item to delete
 * Parameter *to: End of list to delete from.
 */
/*!
 * \brief Clear all entries from beginning up to end.
 *
 *
 * \pre None
 *
 * \post None
 *
 * \param[in] from Item entry to start deleting from.
 * \param[in] to Item entry to delete until, but not including.
 */
static void
_wiimote_config_panel_clear(Elm_Genlist_Item *from, Elm_Genlist_Item *to)
{
#if 0
    Elm_Genlist_Item *next; /*!< GUI element that holds a list entry. */


    /* FIXME: Needs some love because it is broken. */
    if (from != to)
    {
        for (next = elm_genlist_item_next_get(from); next != to; from = next)
        {
            next = elm_genlist_item_next_get(from);
            enna_list2_item_del(from);
        }
    }
#endif /* 0 */
}

/*!
 * \brief Scan for wiimotes and show results.
 *
 * This function scans for wiimotes and starts inserting found devices after
 * the supplied position with an add button.
 *
 * \pre None
 *
 * \post None
 *
 * \param[in]
 * \param[in]
 *
 * \remark Currently we treat all devices in the scan as wiimotes.
 *
 * \return Number of wiimotes found.
 */
static int
_wiimote_config_panel_scan(Elm_Genlist_Item *pos)
{
    int i;                          /*!< Iterator to go over the list. */
    int bdinfo_count;               /*!< Number of wiimotes found. */
    struct cwiid_bdinfo *bdinfo;    /*!< Info storage for bluetooth devices. */


    bdinfo_count =
        cwiid_get_bdinfo_array(-1, 2, -1, &bdinfo, BT_NO_WIIMOTE_FILTER);

    for (i = 0; i < bdinfo_count; i++)
    {
        char ba_str[BA_STR_LEN];
        char *wiimote_name;
        int n;


        n = sizeof(wiimote_name) + sizeof(bdinfo[i].name);

        ba2str(&bdinfo[i].bdaddr, ba_str);
        wiimote_name = ENNA_NEW(char, n);
        snprintf(wiimote_name, n, "[%s] %s", ba_str, bdinfo[i].name);

        if (!wiimote_in_list(ba_str))
        {
            Wiimote_Cfg *w; /*!< Points to wiimote structure in list */


            w = ENNA_NEW(Wiimote_Cfg, 1);
            w->id = -1;
            w->name = bdinfo[i].name;
            strcpy(w->bdaddr, ba_str);
            w->item = enna_list2_item_insert_after(_o_cfg_panel,
                    pos, wiimote_name,
                    _("Device that was found during scan."),
                    NULL, NULL, NULL);

            enna_list2_item_button_add(w->item, NULL, _("Add"),
                    _wiimote_config_panel_add_cb, w);

            mod->scanbot = w->item;
        }
        ENNA_FREE(wiimote_name);
    }

    return bdinfo_count;
}

/*!
 * \brief Scan button callback.
 *
 * This function serves as a callback for the scan button on the configuration
 * panel. First it clears any old scan results and then scans and lists new
 * devices found.
 *
 * \pre None
 *
 * \post None
 *
 * \param[in, out] data Points where to insert scan results.
 * \param[in, out] widget Pointer to the calling widget.
 *
 * \remark FIXME: Adding/deleting an item for the sake of changing its name is
 * ugly and buggy, we shouldn't use it unless someone wants to fix it.
 * TODO: More error reporting once labels can be updated.
 */
static void
_wiimote_config_panel_scan_cb(void *data, Enna_View_List2_Widget *widget)
{
    int bdinfo_count;       /*!< Number of wiimotes found. */
//    Elm_Genlist_Item *item; /*!< GUI element that holds a list entry. */


    _wiimote_config_panel_clear(mod->scantop, mod->scanbot);
    bdinfo_count = _wiimote_config_panel_scan((Elm_Genlist_Item *)data);


#if 0
    /* Needs love, is broken */
    if (bdinfo_count > 0)
    {
        item = enna_list2_item_insert_after(_o_cfg_panel,
                mod->scantop, _("New devices found."),
                _("New devices that where found during the scan."),
                NULL, NULL, NULL);
    }
    else
    {
        item = enna_list2_item_insert_after(_o_cfg_panel,
                mod->scantop, _("No devices found!"),
                _("No devices where found during the scan."),
                NULL, NULL, NULL);
    }
    enna_list2_item_button_add(item, NULL, _("Scan!"),
            _wiimote_config_panel_scan_cb, item);

    enna_list2_item_del(mod->scantop);
    mod->scantop = item;
#endif /* 0 */
}


/****************************************************************************/
/*                           Configuration Panel                            */
/****************************************************************************/

/*!
 * \brief Show enna configuration panel.
 *
 * This function is called whenever the enna configuration panel is entered.
 * Whenever the configuration panel is opened, the entire screen gets rebuilt
 * from scratch.
 *
 * \pre None
 *
 * \post None
 *
 * \param data Unknown, should have been something like enna->evas or he like.
 *
 * \return The created configuration panel.
 */
static Evas_Object *
_wiimote_config_panel_show(void *data)
{
    Eina_List *l;   /*!< Internal list for EINA_LIST_FOREACH. */
    Wiimote_Cfg *w; /*!< Points to wiimote structure in list. */


    _o_cfg_panel = enna_list2_add(enna->evas);

    evas_object_size_hint_align_set(_o_cfg_panel, -1.0, -1.0);
    evas_object_size_hint_weight_set(_o_cfg_panel, 1.0, 1.0);
    evas_object_show(_o_cfg_panel);

    mod->scantop = enna_list2_append(_o_cfg_panel,
            _("Put Wiimote(s) in discoverable mode"),
            _("Put Wiimote(s) in discoverable mode (Press 1+2 on the wiimote)"
                " and hit the scan button."), NULL, NULL, NULL);
    enna_list2_item_button_add(mod->scantop, NULL, _("Scan!"),
            _wiimote_config_panel_scan_cb, mod->scantop);

    mod->result = enna_list2_item_insert_after(_o_cfg_panel, mod->scantop,
            _("Available Wiimote(s)"),
            _("Devices that where loaded from memory."),
            NULL, NULL, NULL);

    mod->scanbot = mod->scantop;

    EINA_LIST_FOREACH(mod->wiimotes, l, w)
    {
        _wiimote_config_panel_wiimote_entry_create(mod->result, w);
    }

    return _o_cfg_panel;
}

/*!
 * \brief Hide enna configuration panel.
 *
 * This function is called whenever the enna configuration panel is left.
 * When closing the application while the configuration panel is open it is
 * assumed the panel gets hidden before closing.
 *
 * \pre None
 *
 * \post None
 *
 * \param Unknown.
 *
 * \remark Some additional cleanup of variables may be required here.
 */
static void
_wiimote_config_panel_hide(void *data)
{
    ENNA_OBJECT_DEL(_o_cfg_panel);
}


/*****************************************************************************/
/*                       Configuration Section Parser                        */
/*****************************************************************************/

/*!
 * \brief Free wiimotes list.
 *
 * This function iterates through the list of wiimotes and free's them.
 *
 * \pre None
 *
 * \post None
 */
static void
cfg_wiimote_section_free(void)
{
    Wiimote_Cfg *w;  /*!< Points to wiimote structure in list. */


    EINA_LIST_FREE(mod->wiimotes, w)
    {
       wiimote_free(w);
    }
}

/*!
 * \brief Load wiimote settings from configuration file.
 *
 * This function loads stored wiimote configurations from the configuration
 * file. First it clears the internal memory structures, then reads a wiimote
 * from the file and adds it to the internal memory structure.
 *
 * \pre None
 *
 * \post None
 *
 * \param[in] section Name of the section to load.
 */
static void
cfg_wiimote_section_load(const char *section)
{
    int i;  /*!< Section Fieldname id. */


    cfg_wiimote_section_free();

    mod->default_wiimote = enna_config_int_get(section, "default_wiimote");
    i = enna_config_int_get(section, "wiimotes");

    /* Count down the number of wiimotes, since our index starts at 0 we
     * subtract 1 to begin with, and then iterate our list down to 0.
     */
    for (i--; i >= 0; i--)
    {
        char field_name[20];    /*!< Name of the field to be loaded. */
        int id;                 /*!< ID of the loaded wiimote. */
        Eina_Bool connect;      /*!< Connection status of stored wiimote. */
        const char *name;       /*!< Name of the stored wiimote. */
        const char *bdaddr;     /*!< Bluetooth address string of the stored
                                  wiimote. */


        snprintf(field_name, sizeof(field_name), "wiimote_%02d_id", i);
        id = enna_config_int_get(section, field_name);

        snprintf(field_name, sizeof(field_name), "wiimote_%02d_name", i);
        name = enna_config_string_get(section, field_name);

        snprintf(field_name, sizeof(field_name), "wiimote_%02d_bdaddr", i);
        bdaddr = enna_config_string_get(section, field_name);

        snprintf(field_name, sizeof(field_name), "wiimote_%02d_connect", i);
        connect = enna_config_bool_get(section, field_name);

        wiimote_add(id, name, bdaddr);

        if (connect)
        {
            /* Connect to previously connected wiimotes on load.
             * This currently can't work, unless a wiimote is in discoverable
             * mode during load. I suppose this will work if the wiimote is
             * 'bound' to our bluetooth hardware ID, but this is untested.
             */
//            wiimote_connect(id);
        }
    }
}

/*!
 * \brief Safe wiimote settings to file.
 *
 * This function saves wiimote configurations from the internal memory
 * structure.
 *
 * \pre None
 *
 * \post None
 *
 * \param[in] section Name of the section to be saved.
 *
 * \remark TODO: purge unused entries from file. Currently unused entries are
 * set to some meaningless default values. They aren't used but should be
 * cleaned somehow.
 */
static void
cfg_wiimote_section_save(const char *section)
{
    Eina_List *l;           /*!< Internal list for EINA_LIST_FOREACH. */
    Wiimote_Cfg *w;         /*!< Points to wiimote structure in list. */
    int i;                  /*!< Section fieldname id. */
    int load_cnt;           /*!< Amount of wiimotes that where loaded. */
    char field_name[20];    /*!< Name of the field to be loaded. */


    load_cnt = enna_config_int_get(section, "wiimotes");
    enna_config_int_set(section, "default_wiimote", mod->default_wiimote);
    enna_config_int_set(section, "wiimotes", eina_list_count(mod->wiimotes));

    i = 0;
    EINA_LIST_FOREACH(mod->wiimotes, l, w)
    {
        snprintf(field_name, sizeof(field_name), "wiimote_%02d_id", i);
        enna_config_int_set(section, field_name, w->id);
        snprintf(field_name, sizeof(field_name), "wiimote_%02d_name", i);
        if (!w->name)
        {
            w->name = strdup(_("Unnamed Wiimote"));
        }
        enna_config_string_set(section, field_name, w->name);
        snprintf(field_name, sizeof(field_name), "wiimote_%02d_bdaddr", i);
        enna_config_string_set(section, field_name, w->bdaddr);
        snprintf(field_name, sizeof(field_name), "wiimote_%02d_connect", i);
        w->connected = EINA_FALSE;
        enna_config_bool_set(section, field_name, w->connected);

        i++;
    }

    /* Once saving of new values is complete, purge the excess ones. */
    for (; load_cnt > i; i++)
    {
        enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME, "Purging slot %d.", i);
        snprintf(field_name, sizeof(field_name), "wiimote_%02d_id", i);
        enna_config_int_set(section, field_name, -1);
        snprintf(field_name, sizeof(field_name), "wiimote_%02d_name", i);
        enna_config_string_set(section, field_name, "Empty Slot");
        snprintf(field_name, sizeof(field_name), "wiimote_%02d_bdaddr", i);
        enna_config_string_set(section, field_name, "[00:00:00:00:00]");
        snprintf(field_name, sizeof(field_name), "wiimote_%02d_connect", i);
        enna_config_bool_set(section, field_name, EINA_FALSE);
    }
}

/*!
 * \brief Set default values.
 *
 * This function loads a default entry into the internal memory structures. It
 * adds an non-existing wiimote.
 *
 * \pre None
 *
 * \post None
 */
static void
cfg_wiimote_section_set_default(void)
{
    cfg_wiimote_section_free();

    wiimote_add(0, _("Unconfigured Wiimote"), "00:00:00:00:00");

    mod->default_wiimote = 0;
}

/*!
 * \brief Function pointer struct for the configuration parser.
 *
 * This structure holds the name of the section and the pointers to the
 * functions to load, save, set default values and free wiimote configuration
 * data to the configuration file.
 */
static Enna_Config_Section_Parser cfg_wiimote_section = {
    "wiimote",
    cfg_wiimote_section_load,
    cfg_wiimote_section_save,
    cfg_wiimote_section_set_default,
    cfg_wiimote_section_free,
};


/****************************************************************************/
/*                            Module interface                              */
/****************************************************************************/

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
/*!
 * \brief Define to give the module unique prefixes.
 */
#define MOD_PREFIX enna_mod_input_wiimote
#endif /* USE_STATIC_MODULES */

/*!
 * \brief Initialize wiimote module.
 *
 * This function initializes the modules structures and registers the
 * fileparser and configuration panel.
 *
 * \pre None
 *
 * \post None
 *
 * \param[in, out] em Unknown.
 */
static void
module_init(Enna_Module *em)
{
    if (em)
    {
        mod = ENNA_NEW(Enna_Module_Wiimote, 1);
        em->mod = mod;

        enna_config_section_parser_register(&cfg_wiimote_section);
        _config_panel = enna_config_panel_register(_("Wiimote configuration"),
            "icon/wiimote", _wiimote_config_panel_show,
            _wiimote_config_panel_hide, NULL);
    }
}

/*!
 * \brief Un-initialize wiimote module.
 *
 *
 * \pre None
 *
 * \post None
 *
 * \param[in, out] em Unknown.
 *
 * \remark It is currently unknown which memories still need to be freed.
 */
static void
module_shutdown(Enna_Module *em)
{
    Enna_Module_Wiimote *mod;    /*!< Contains module specific data. */


    mod = em->mod;

    enna_config_panel_unregister(_config_panel);
    enna_config_section_parser_unregister(&cfg_wiimote_section);
}

/*!
 * \brief Module API definition.
 *
 * This structure exports module information.
 *
 */
Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    ENNA_MODULE_NAME,
    N_("Wiimote"),
    NULL,
    N_("Module to control enna from your wiimote"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla.",
    {
        module_init,
        module_shutdown
    }
};
