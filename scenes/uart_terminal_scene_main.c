#include "../uart_terminal_app_i.h"
#include <dolphin/dolphin.h>

UART_TerminalItem mainmenu[NUM_MAIN_ITEMS] = {
  {"Targets",
  {""},
  1,
  {""},
  TOGGLE_ARGS,
  FOCUS_CONSOLE_END,
  NO_TIP,
  true},
  {"Packets",
  {""},
  1,
  {""},
  TOGGLE_ARGS,
  FOCUS_CONSOLE_END,
  NO_TIP,
  true},
  {"Attacks",
  {""},
  1,
  {""},
  TOGGLE_ARGS,
  FOCUS_CONSOLE_END,
  NO_TIP,
  true},
  {"Settings",
  {""},
  1,
  {""},
  TOGGLE_ARGS,
  FOCUS_CONSOLE_END,
  NO_TIP,
  true},
  {"Help",
  {""},
  1,
  {""},
  TOGGLE_ARGS,
  FOCUS_CONSOLE_END,
  NO_TIP,
  true},
  {"Console",
  {"View", "Clear", "Send Command"},
  3,
  {"", "cls", " "},
  TOGGLE_ARGS,
  FOCUS_CONSOLE_END,
  NO_TIP,
  false}
};

static void displaySubmenu(UART_TerminalApp *app, UART_TerminalItem *item) {
    int newScene = -1;
    if (!strcmp(item->item_string, "Targets")) {
        // Targets menu
        newScene = UART_TerminalSceneTargets;
    } else if (!strcmp(item->item_string, "Packets")) {
        newScene = UART_TerminalScenePackets;
    } else if (!strcmp(item->item_string, "Attacks")) {
        newScene = UART_TerminalSceneAttacks;
    } else if (!strcmp(item->item_string, "Settings")) {
        newScene = UART_TerminalSceneSettings;
    } else if (!strcmp(item->item_string, "Help")) {
        newScene = UART_TerminalSceneHelp;
    }
    if (newScene < 0) {
        return;
    }
    scene_manager_next_scene(app->scene_manager, newScene);
}

/* Callback when an option is selected */
static void uart_terminal_scene_main_var_list_enter_callback(void* context, uint32_t index) {
    furi_assert(context);
    UART_TerminalApp* app = context;
    UART_TerminalItem *item = NULL;
    const int selected_option_index = app->selected_menu_options[GRAVITY_MENU_MAIN][index];
    furi_assert(index < NUM_MAIN_ITEMS);
    app->selected_menu_items[GRAVITY_MENU_MAIN] = index;

    item = &mainmenu[index];

    /* Are we displaying a submenu or executing something? */
    if (item->isSubMenu) {
        /* Display next scene */
        displaySubmenu(app, item);
    } else {
        /* Run a command */
        dolphin_deed(DolphinDeedGpioUartBridge);
        furi_assert(selected_option_index < item->num_options_menu);
        app->selected_tx_string = item->actual_commands[selected_option_index];
        /* Don't clear screen if command is an empty string */
        app->is_command = (strlen(app->selected_tx_string) > 0);
        app->is_custom_tx_string = false;
        app->focus_console_start = (item->focus_console == FOCUS_CONSOLE_TOGGLE) ?
                                   (selected_option_index == 0) :
                                   item->focus_console;
        app->show_stopscan_tip = item->show_stopscan_tip;

        /* GRAVITY: For TOGGLE_ARGS display a keyboard if actual_command ends with ' ' */
        int cmdLen = strlen(app->selected_tx_string);
        bool needs_keyboard = ((item->needs_keyboard == INPUT_ARGS) ||
                                (item->needs_keyboard == TOGGLE_ARGS &&
                                (app->selected_tx_string[cmdLen-1] == ' ')));
        /* Initialise the serial console */
        uart_terminal_uart_tx((uint8_t*)("\n"), 1);

        if(needs_keyboard) {
            view_dispatcher_send_custom_event(app->view_dispatcher, UART_TerminalEventStartKeyboard);
        } else {
            view_dispatcher_send_custom_event(app->view_dispatcher, UART_TerminalEventStartConsole);
        }
    }
}

/* Callback when a selected option is changed (I Think) */
static void uart_terminal_scene_main_var_list_change_callback(VariableItem* item) {
    furi_assert(item);

    UART_TerminalApp* app = variable_item_get_context(item);
    furi_assert(app);

    if (app->selected_menu_items[GRAVITY_MENU_MAIN] >= NUM_MAIN_ITEMS) {
        app->selected_menu_items[GRAVITY_MENU_MAIN] = 0;
    }

    const UART_TerminalItem* menu_item = &mainmenu[app->selected_menu_items[GRAVITY_MENU_MAIN]];
    uint8_t item_index = variable_item_get_current_value_index(item);
    furi_assert(item_index < menu_item->num_options_menu);
    variable_item_set_current_value_text(item, menu_item->options_menu[item_index]);
    app->selected_menu_options[GRAVITY_MENU_MAIN][app->selected_menu_items[GRAVITY_MENU_MAIN]] = item_index;
}

/* Callback on entering the scene (initialisation) */
void uart_terminal_scene_main_on_enter(void* context) {
    UART_TerminalApp* app = context;
    VariableItemList* var_item_list = app->main_menu_list;
    VariableItem *item;

    variable_item_list_set_enter_callback(
        var_item_list, uart_terminal_scene_main_var_list_enter_callback, app);

    app->currentMenu = GRAVITY_MENU_MAIN;
    for(int i = 0; i < NUM_MAIN_ITEMS; ++i) {
        item = variable_item_list_add(
            var_item_list,
            mainmenu[i].item_string,
            mainmenu[i].num_options_menu,
            uart_terminal_scene_main_var_list_change_callback,
            app);
        variable_item_set_current_value_index(item, app->selected_menu_options[GRAVITY_MENU_MAIN][i]);
        variable_item_set_current_value_text(
            item, mainmenu[i].options_menu[app->selected_menu_options[GRAVITY_MENU_MAIN][i]]);
    }
    variable_item_list_set_selected_item(var_item_list, app->selected_menu_items[GRAVITY_MENU_MAIN]);

    view_dispatcher_switch_to_view(app->view_dispatcher, Gravity_AppViewMainMenu);
}

/* Event handler callback - Handle scene change and tick events */
bool uart_terminal_scene_main_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UART_TerminalApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        int nextScene = 0;
        if (event.event == UART_TerminalEventStartKeyboard) {
            nextScene = UART_TerminalAppViewTextInput;
        } else if (event.event == UART_TerminalEventStartConsole) {
            nextScene = UART_TerminalAppViewConsoleOutput;
        }
        scene_manager_next_scene(app->scene_manager, nextScene);
        consumed = true;
    } else if(event.type == SceneManagerEventTypeTick) {
        app->selected_menu_items[GRAVITY_MENU_MAIN] = variable_item_list_get_selected_item_index(app->main_menu_list);
        consumed = true;
    }
    return consumed;
}

/* Clean up on exit */
void uart_terminal_scene_main_on_exit(void* context) {
    UART_TerminalApp* app = context;
    variable_item_list_reset(app->main_menu_list);
}
