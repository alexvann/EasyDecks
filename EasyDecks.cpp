#include <SokuLib.hpp>

/*#######################################################*/

THMemPatcher2 mod;

hook_t<vtable::Scene::SelectSV, offset::Scene::Process> select_sv_hook;
hook_t<vtable::Scene::SelectCL, offset::Scene::Process> select_cl_hook;
hook_t<vtable::Menu::ProfileSelect, offset::Menu::Destruct> menu_destruct_hook;

jmp_t<0x0044CC48, 6> profile_menu_process;

patch_t<0x0042FA43, 0xE9, 0x37, 0x04, 0x00, 0x00, 0x90> disable_editing_score123_dat;
patch_t<0x004559D2, 0xB8, 0x00, 0x00> disable_keys_for_server;
patch_t<0x00454CC6, 0x90, 0x90, 0x90> disable_keys_for_client;

constexpr size_t PROFILE_MENU_SIZE = 452;
void* pMenu = nullptr;
enum ProfileMenuItem : uint32_t { P1_PROFILE_SET, P2_PROFILE_SET, P1_DECK_BUILDER, P2_DECK_BUILDER, P1_KEY_CONFIG, P2_KEY_CONFIG, GO_TO_TITLE, EXIT };

/*#######################################################*/

bool create_profile_menu() {
	Soku::play_se_wave_buffer(SoundID::MENU_SELECT);
	auto pBuffer = Soku::new_fn(PROFILE_MENU_SIZE);
	if (pBuffer) {
		pMenu = Soku::create_profile_menu(pBuffer);
		Soku::activate_menu(pMenu);
		return true;
	}
	return false;
}

bool is_button_clicked() {
	return (Soku::key_manager.inputs.C == 1 || Soku::key_manager_ex.inputs.C == 1);
}

void disable_keys_in_character_select() {
	Soku::enable_esc(false);
	mod.ApplyPatches();
	Soku::hide_modal_window();
}

void enable_keys_in_character_select() {
	Soku::enable_esc(true);
	mod.RestorePatches();
	pMenu = nullptr;
}

void menu_process() {
	if (!pMenu && is_button_clicked() && create_profile_menu())
		disable_keys_in_character_select();
}

/*#######################################################*/

def_hook(select_sv_hook)(void* _this) {
	menu_process();
	return select_sv_hook(_this);
}

def_hook(select_cl_hook)(void* _this) {
	menu_process();
	return select_cl_hook(_this);
}

def_hook(menu_destruct_hook)(void* _this, int, unsigned char a) {
	if (pMenu == _this)
		enable_keys_in_character_select();
	return menu_destruct_hook(_this, a);
}

def_jmp(profile_menu_process) {
	__asm {
		CMP EAX, P1_DECK_BUILDER
		JE default_action
		CMP EAX, P2_DECK_BUILDER
		JE default_action
		CMP EAX, EXIT
		JE default_action
		//no_action:
		MOV AL, 1
		POP ECX
		POP EDI
		POP ESI
		POP EBP
		POP EBX
		ADD ESP, 0x10
		RETN
	}
	default_action: 
	jmp_back(profile_menu_process)
}

/*#######################################################*/

bool Initialize(HMODULE, HMODULE) {
	mod.AddHook(select_sv_hook, true);
	mod.AddHook(select_cl_hook, true);
	mod.AddHook(menu_destruct_hook, true);
	mod.AddPatch(disable_editing_score123_dat, true);
	mod.ApplyPatches();

	mod.AddPatch(disable_keys_for_server);
	mod.AddPatch(disable_keys_for_client);
	mod.AddDetourJump(profile_menu_process);
	return true;
}

void AtExit() {
	enable_keys_in_character_select();
	mod.ClearPatches();
}