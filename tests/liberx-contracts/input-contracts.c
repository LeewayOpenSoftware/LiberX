#define COBJMACROS
#define CONST_VTABLE
#define INITGUID
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <xinput.h>
#include <dinput.h>

#define TEST_VERSION 1

static int fail_count, skip_count, pass_count;

static void report(const char *name, const char *status, const char *fmt, ...)
{
    char detail[256];
    va_list args;

    va_start(args, fmt);
    vsnprintf(detail, sizeof(detail), fmt, args);
    va_end(args);
    printf("INPUT_CONTRACTS v%d %s %s %s\n", TEST_VERSION, name, status, detail);
    if (status[0] == 'F') ++fail_count;
    else if (status[0] == 'S') ++skip_count;
    else ++pass_count;
}

static void check(const char *name, BOOL ok, const char *fmt, ...)
{
    char detail[256];
    va_list args;

    va_start(args, fmt);
    vsnprintf(detail, sizeof(detail), fmt, args);
    va_end(args);
    report(name, ok ? "PASS" : "FAIL", "%s", detail);
}

typedef DWORD (WINAPI *xinput_get_state_func)(DWORD, XINPUT_STATE *);
typedef DWORD (WINAPI *xinput_set_state_func)(DWORD, XINPUT_VIBRATION *);
typedef DWORD (WINAPI *dinput_create_func)(HINSTANCE, DWORD, REFIID, void **, void *);

static void test_xinput_unsupported(void)
{
    HMODULE xinput;
    xinput_get_state_func get_state;
    XINPUT_STATE state;
    DWORD result;

    xinput = LoadLibraryA("xinput1_4.dll");
    if (!xinput) {
        report("xinput_load", "SKIP", "xinput1_4.dll not found");
        return;
    }

    get_state = (xinput_get_state_func)GetProcAddress(xinput, "XInputGetState");
    if (!get_state) {
        report("xinput_export", "FAIL", "XInputGetState not found");
        FreeLibrary(xinput);
        return;
    }

    /* Non-existent controller should fail honestly */
    result = get_state(255, &state);
    check("xinput_invalid_user", result == ERROR_DEVICE_NOT_CONNECTED || result == ERROR_NOT_SUPPORTED,
            "invalid user: result=%lu", result);

    FreeLibrary(xinput);
}

static void test_dinput_create(void)
{
    HMODULE dinput8;
    dinput_create_func create_func;
    IDirectInput8A *dinput = NULL;
    HRESULT hr;

    dinput8 = LoadLibraryA("dinput8.dll");
    if (!dinput8) {
        report("dinput_load", "SKIP", "dinput8.dll not found");
        return;
    }

    create_func = (dinput_create_func)GetProcAddress(dinput8, "DirectInput8Create");
    if (!create_func) {
        report("dinput_export", "FAIL", "DirectInput8Create not found");
        FreeLibrary(dinput8);
        return;
    }

    hr = create_func(GetModuleHandleA(NULL), DIRECTINPUT_VERSION, &IID_IDirectInput8A, (void **)&dinput, NULL);
    check("dinput_create", SUCCEEDED(hr),
            "create: hr=%lx", hr);

    if (SUCCEEDED(hr) && dinput)
    {
        hr = IDirectInput8_EnumDevices(dinput, DI8DEVCLASS_GAMECTRL, NULL, NULL, DIEDFL_ALLDEVICES);
        check("dinput_enum", hr == E_INVALIDARG,
                "enum (no callback) must fail: hr=%lx", hr);

        IDirectInput8_Release(dinput);
    }

    FreeLibrary(dinput8);
}

static void test_dinput_unsupported_operations(void)
{
    HMODULE dinput8;
    dinput_create_func create_func;
    IDirectInput8A *dinput = NULL;
    HRESULT hr;

    dinput8 = LoadLibraryA("dinput8.dll");
    if (!dinput8) {
        report("dinput_unsupported", "SKIP", "dinput8.dll not found");
        return;
    }

    create_func = (dinput_create_func)GetProcAddress(dinput8, "DirectInput8Create");
    if (!create_func) {
        report("dinput_unsupported", "SKIP", "DirectInput8Create export");
        FreeLibrary(dinput8);
        return;
    }

    hr = create_func(GetModuleHandleA(NULL), DIRECTINPUT_VERSION, &IID_IDirectInput8A, (void **)&dinput, NULL);
    if (FAILED(hr)) {
        report("dinput_unsupported", "SKIP", "DirectInput8Create failed");
        FreeLibrary(dinput8);
        return;
    }

    if (dinput)
    {
        hr = IDirectInput8_SendDeviceData(dinput, 0, NULL, NULL, 0);
        check("dinput_senddevicedata", hr == DIERR_UNSUPPORTED,
                "SendDeviceData must return unsupported: hr=%lx", hr);

        IDirectInput8_Release(dinput);
    }

    FreeLibrary(dinput8);
}

static void test_hid_available(void)
{
    HMODULE setupapi, hid;
    BOOL has_setupapi, has_hid;

    setupapi = LoadLibraryA("setupapi.dll");
    hid = LoadLibraryA("hid.dll");

    has_setupapi = setupapi != NULL;
    has_hid = hid != NULL;

    check("hid_libraries", has_setupapi && has_hid,
            "setupapi=%d hid=%d", has_setupapi, has_hid);

    if (setupapi) FreeLibrary(setupapi);
    if (hid) FreeLibrary(hid);
}

int main(void)
{
    test_xinput_unsupported();
    test_dinput_create();
    test_dinput_unsupported_operations();
    test_hid_available();

    printf("INPUT_CONTRACTS v%d SUMMARY PASS=%d FAIL=%d SKIP=%d\n", TEST_VERSION, pass_count, fail_count, skip_count);
    return fail_count;
}
