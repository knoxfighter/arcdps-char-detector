/**
 * Before installing ixwebsocket with vcpkg, change the windows x64 linkage to static, else the dll cant be loaded:
 *
 * C:\vcpkg\triplets\x64-windows.cmake
 * ```
 * #set(VCPKG_LIBRARY_LINKAGE static);
 * ```
 *
 * In theory installing the static packaged, could also work, but VS was not able to locate them correctly.
 */

// dllmain.cpp : Definiert den Einstiegspunkt für die DLL-Anwendung.
#include <Windows.h>
#include <d3d9.h>
#include <cstdint>
#include <vector>
#include <string>

#include "arcdps_structs.h"
#include "imgui/imgui.h"
#include "SettingsUI.h"
#include "StreamLabs.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

char* arcvers;
arcdps_exports arc_exports;
bool show_options = false;
SettingsUI settingsUI;

typedef uint64_t(*arc_export_func_u64)();
auto arc_dll = LoadLibraryA("d3d9.dll");
// arc options
auto arc_export_e6 = (arc_export_func_u64)GetProcAddress(arc_dll, "e6");
bool arc_hide_all = false;
bool arc_panel_always_draw = false;
bool arc_movelock_altui = false;
bool arc_clicklock_altui = false;
bool arc_window_fastclose = false;

// arc keyboard modifier
auto arc_export_e7 = (arc_export_func_u64)GetProcAddress(arc_dll, "e7");
DWORD arc_global_mod1 = 0;
DWORD arc_global_mod2 = 0;
DWORD arc_global_mod_multi = 0;

/* window callback -- return is assigned to umsg (return zero to not be processed by arcdps or game) */
uintptr_t mod_wnd(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	auto const io = &ImGui::GetIO();

	switch (uMsg)
	{
	case WM_KEYUP:
	{
		const int vkey = (int)wParam;
		io->KeysDown[vkey] = 0;
		if (vkey == VK_CONTROL)
		{
			io->KeyCtrl = false;
		}
		else if (vkey == VK_MENU)
		{
			io->KeyAlt = false;
		}
		else if (vkey == VK_SHIFT)
		{
			io->KeyShift = false;
		}
		break;
	}
	case WM_KEYDOWN:
	{
		const int vkey = (int)wParam;
		io->KeysDown[vkey] = 1;
		if (vkey == VK_CONTROL)
		{
			io->KeyCtrl = true;
		}
		else if (vkey == VK_MENU)
		{
			io->KeyAlt = true;
		}
		else if (vkey == VK_SHIFT)
		{
			io->KeyShift = true;
		}
		break;
	}
	case WM_SYSKEYUP:
	{
		const int vkey = (int)wParam;
		io->KeysDown[vkey] = 0;
		if (vkey == VK_CONTROL)
		{
			io->KeyCtrl = false;
		}
		else if (vkey == VK_MENU)
		{
			io->KeyAlt = false;
		}
		else if (vkey == VK_SHIFT)
		{
			io->KeyShift = false;
		}
		break;
	}
	case WM_SYSKEYDOWN:
	{
		const int vkey = (int)wParam;
		io->KeysDown[vkey] = 1;
		if (vkey == VK_CONTROL)
		{
			io->KeyCtrl = true;
		}
		else if (vkey == VK_MENU)
		{
			io->KeyAlt = true;
		}
		else if (vkey == VK_SHIFT)
		{
			io->KeyShift = true;
		}
		break;
	}
	case WM_ACTIVATEAPP:
	{
		if (!wParam)
		{
			io->KeysDown[arc_global_mod1] = false;
			io->KeysDown[arc_global_mod2] = false;
		}
		break;
	}
	break;
	}

	return uMsg;
}

/* combat callback -- may be called asynchronously. return ignored */
/* one participant will be party/squad, or minion of. no spawn statechange events. despawn statechange only on marked boss npcs */
uintptr_t mod_combat(cbtevent* ev, ag* src, ag* dst, char* skillname, uint64_t id, uint64_t revision) {
	/* ev is null. dst will only be valid on tracking add. skillname will also be null */
	if (!ev) {

		/* notify tracking change */
		if (!src->elite) {

			/* add */
			if (src->prof) {
				StreamLabs& streamLabs = StreamLabs::instance();
				const std::string username = streamLabs.getUsername();
				if (username.empty()) {
					// this is the first occurrence, save the username...
					streamLabs.setUsername(dst->name);

					streamLabs.setCurrentCharacter(src->name);
				} else if (username == dst->name) {
					streamLabs.setCurrentCharacter(src->name);
				}
			}
		}
	}
	return 0;
}

uintptr_t mod_options()
{
	if (ImGui::BeginMenu("StreamLabs"))
	{
		//ImGui::Checkbox("Mechanics Log", &show_app_log);
		//ImGui::Checkbox("Mechanics Chart", &show_app_chart);
		ImGui::Checkbox("Options", &show_options);
		ImGui::EndMenu();
	}

	return 0;
}

bool modsPressed()
{
	auto io = &ImGui::GetIO();

	return io->KeysDown[arc_global_mod1] && io->KeysDown[arc_global_mod2];
}

bool canMoveWindows()
{
	if (!arc_movelock_altui)
	{
		return true;
	}
	else
	{
		return modsPressed();
	}
}

void ShowOptions(bool* p_open) {
	if (show_options) {
		settingsUI.draw("StreamLabs options", p_open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | (!canMoveWindows() ? ImGuiWindowFlags_NoMove : 0));
	}
}

void readArcExports()
{
	uint64_t e6_result = arc_export_e6();
	uint64_t e7_result = arc_export_e7();

	arc_hide_all = (e6_result & 0x01);
	arc_panel_always_draw = (e6_result & 0x02);
	arc_movelock_altui = (e6_result & 0x04);
	arc_clicklock_altui = (e6_result & 0x08);
	arc_window_fastclose = (e6_result & 0x10);


	uint16_t* ra = (uint16_t*)&e7_result;
	if (ra)
	{
		arc_global_mod1 = ra[0];
		arc_global_mod2 = ra[1];
		arc_global_mod_multi = ra[2];
	}
}

uintptr_t mod_imgui(uint32_t not_charsel_or_loading)
{
	readArcExports();

	//if (!not_charsel_or_loading) return 0;

	ShowOptions(&show_options);

	return 0;
}


/* initialize mod -- return table that arcdps will use for callbacks */
arcdps_exports* mod_init() {
	/* for arcdps */
	memset(&arc_exports, 0, sizeof(arcdps_exports));
	arc_exports.sig = 0x0564DC406004E4FD;
	arc_exports.size = sizeof(arcdps_exports);
	arc_exports.out_name = "currentChar";
	arc_exports.out_build = "0.1";
	arc_exports.wnd_nofilter = mod_wnd;
	arc_exports.combat = mod_combat;
	arc_exports.imgui = mod_imgui;
	arc_exports.options_end = mod_options;
	//arc_exports.size = (uintptr_t)"error message if you decide to not load, sig must be 0";
	return &arc_exports;
}

/* export -- arcdps looks for this exported function and calls the address it returns on client load */
extern "C" __declspec(dllexport) void* get_init_addr(char* arcversionstr, void* imguicontext) {
    arcvers = arcversionstr;
	ImGui::SetCurrentContext((ImGuiContext*)imguicontext);
    return mod_init;
}

/* release mod -- return ignored */
uintptr_t mod_release() {
	return 0;
}

/* export -- arcdps looks for this exported function and calls the address it returns on client exit */
extern "C" __declspec(dllexport) void* get_release_addr() {
	arcvers = nullptr;
    return mod_release;
}
