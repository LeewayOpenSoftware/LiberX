#define COBJMACROS
#define CONST_VTABLE
#define INITGUID
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <initguid.h>
#include <d3d12.h>
#include <dxgi.h>
#include <dwrite.h>

#define CONTRACT_VERSION 1
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

DEFINE_GUID(iid_dxgi_factory1, 0x770aae78, 0xf26f, 0x4dba, 0xa8, 0x29, 0x25, 0x3c, 0x83, 0xd1, 0xb3, 0x87);
DEFINE_GUID(iid_dwrite_factory, 0xb859ee5a, 0xd838, 0x4b5b, 0xa2, 0xe8, 0x1a, 0xdc, 0x7d, 0x93, 0xdb, 0x48);
DEFINE_GUID(iid_d2d_factory, 0x06152247, 0x6f50, 0x465a, 0x92, 0x45, 0x11, 0x8b, 0xfd, 0x3b, 0x60, 0x07);
DEFINE_GUID(iid_d3d12_device, 0x189819f1, 0x1db6, 0x4b57, 0xbe, 0x54, 0x18, 0x21, 0x33, 0x9b, 0x85, 0xf7);
DEFINE_GUID(iid_unknown_probe, 0x5b7a1c1e, 0x9d3a, 0x4a52, 0x8f, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66);

#define ASSERT_SLOT(name, vtbl, member, index) \
    typedef char slot_##name[(offsetof(vtbl, member) / sizeof(void *) == (index)) ? 1 : -1]

ASSERT_SLOT(dxgi_object_get_parent, IDXGIObjectVtbl, GetParent, 6);
ASSERT_SLOT(dxgi_factory_enum_adapters, IDXGIFactoryVtbl, EnumAdapters, 7);
ASSERT_SLOT(dxgi_factory_create_swap_chain, IDXGIFactoryVtbl, CreateSwapChain, 10);
ASSERT_SLOT(dxgi_factory1_enum_adapters1, IDXGIFactory1Vtbl, EnumAdapters1, 12);
ASSERT_SLOT(d3d12_object_set_name, ID3D12ObjectVtbl, SetName, 6);
ASSERT_SLOT(d3d12_device_get_node_count, ID3D12DeviceVtbl, GetNodeCount, 7);
ASSERT_SLOT(d3d12_device_create_command_queue, ID3D12DeviceVtbl, CreateCommandQueue, 8);
ASSERT_SLOT(dwrite_factory_get_system_font_collection, IDWriteFactoryVtbl, GetSystemFontCollection, 3);

static int fail_count, skip_count, pass_count;

static void report(const char *dll, const char *name, const char *status, const char *fmt, ...)
{
    char detail[256];
    va_list args;

    va_start(args, fmt);
    vsnprintf(detail, sizeof(detail), fmt, args);
    va_end(args);
    printf("CONTRACT v%d %s %s %s %s\n", CONTRACT_VERSION, dll, name, status, detail);
    if (status[0] == 'F') ++fail_count;
    else if (status[0] == 'S') ++skip_count;
    else ++pass_count;
}

static void check(const char *dll, const char *name, BOOL ok, const char *fmt, ...)
{
    char detail[256];
    va_list args;

    va_start(args, fmt);
    vsnprintf(detail, sizeof(detail), fmt, args);
    va_end(args);
    report(dll, name, ok ? "PASS" : "FAIL", "%s", detail);
}

static const struct { const char *dll; const char *exports[8]; } export_table[] =
{
    {"d3d11.dll", {"D3D11CreateDevice", "D3D11CreateDeviceAndSwapChain"}},
    {"d3d12.dll", {"D3D12CreateDevice", "D3D12GetDebugInterface", "D3D12SerializeRootSignature",
            "D3D12CreateRootSignatureDeserializer", "D3D12EnableExperimentalFeatures"}},
    {"dxgi.dll", {"CreateDXGIFactory", "CreateDXGIFactory1", "CreateDXGIFactory2"}},
    {"dxcore.dll", {"DXCoreCreateAdapterFactory"}},
    {"d2d1.dll", {"D2D1CreateFactory", "D2D1CreateDevice"}},
    {"dwrite.dll", {"DWriteCreateFactory"}},
    {"d3d9.dll", {"Direct3DCreate9", "Direct3DCreate9Ex"}},
    {"ddraw.dll", {"DirectDrawCreate", "DirectDrawCreateEx"}},
    {"dsound.dll", {"DirectSoundCreate", "DirectSoundCreate8"}},
    {"dinput8.dll", {"DirectInput8Create"}},
    {"xinput1_4.dll", {"XInputGetState", "XInputSetState", "XInputGetCapabilities"}},
    {"d3dcompiler_47.dll", {"D3DCompile", "D3DReflect"}},
};

static void test_exports(void)
{
    unsigned int i, j;

    for (i = 0; i < ARRAY_SIZE(export_table); ++i)
    {
        HMODULE module = LoadLibraryA(export_table[i].dll);

        if (!module)
        {
            report(export_table[i].dll, "load", "FAIL", "error %lu", GetLastError());
            continue;
        }
        for (j = 0; j < ARRAY_SIZE(export_table[i].exports) && export_table[i].exports[j]; ++j)
            check(export_table[i].dll, export_table[i].exports[j], !!GetProcAddress(module, export_table[i].exports[j]),
                    "export");
    }
}

typedef HRESULT (WINAPI *create_dxgi_func)(REFIID, void **);
typedef HRESULT (WINAPI *create_dwrite_func)(UINT, REFIID, IUnknown **);
typedef HRESULT (WINAPI *create_d2d_func)(UINT, REFIID, const void *, void **);
typedef HRESULT (WINAPI *create_d3d12_func)(IUnknown *, UINT, REFIID, void **);
typedef HRESULT (WINAPI *create_d3d11_func)(void *, UINT, HMODULE, UINT, const UINT *, UINT, UINT, void **,
        UINT *, void **);

static void test_unknown_contract(const char *dll, const char *what, IUnknown *object)
{
    IUnknown *other = (IUnknown *)(ULONG_PTR)1;
    IUnknown *same = NULL;
    ULONG ref;
    HRESULT hr;

    hr = IUnknown_QueryInterface(object, &iid_unknown_probe, (void **)&other);
    check(dll, what, hr == E_NOINTERFACE && !other, "QI unknown iid: hr=%#lx out=%p", hr, other);

    hr = IUnknown_QueryInterface(object, &IID_IUnknown, (void **)&same);
    check(dll, what, hr == S_OK && same, "QI IUnknown: hr=%#lx", hr);
    if (same)
    {
        ref = IUnknown_AddRef(object);
        check(dll, what, ref >= 2, "AddRef after QI: %lu", ref);
        IUnknown_Release(object);
        IUnknown_Release(same);
    }
}

static void test_dxgi(void)
{
    create_dxgi_func create;
    IUnknown *factory = (IUnknown *)(ULONG_PTR)1;
    HMODULE module = LoadLibraryA("dxgi.dll");
    HRESULT hr;

    if (!module || !(create = (create_dxgi_func)GetProcAddress(module, "CreateDXGIFactory1")))
    {
        report("dxgi.dll", "factory", "SKIP", "dxgi.dll or CreateDXGIFactory1 missing");
        return;
    }

    hr = create(&iid_unknown_probe, (void **)&factory);
    check("dxgi.dll", "factory-bad-iid", FAILED(hr) && !factory, "hr=%#lx out=%p", hr, factory);

    factory = NULL;
    hr = create(&iid_dxgi_factory1, (void **)&factory);
    if (FAILED(hr))
    {
        report("dxgi.dll", "factory", "FAIL", "hr=%#lx", hr);
        return;
    }
    test_unknown_contract("dxgi.dll", "factory", factory);
    IUnknown_Release(factory);
}

static void test_dwrite(void)
{
    create_dwrite_func create;
    IUnknown *factory = (IUnknown *)(ULONG_PTR)1;
    HMODULE module = LoadLibraryA("dwrite.dll");
    HRESULT hr;

    if (!module || !(create = (create_dwrite_func)GetProcAddress(module, "DWriteCreateFactory")))
    {
        report("dwrite.dll", "factory", "SKIP", "dwrite.dll or DWriteCreateFactory missing");
        return;
    }

    hr = create(99, &iid_dwrite_factory, &factory);
    check("dwrite.dll", "factory-bad-type", FAILED(hr) && !factory, "hr=%#lx out=%p", hr, factory);

    factory = NULL;
    hr = create(DWRITE_FACTORY_TYPE_SHARED, &iid_dwrite_factory, &factory);
    if (FAILED(hr))
    {
        report("dwrite.dll", "factory", "FAIL", "hr=%#lx", hr);
        return;
    }
    test_unknown_contract("dwrite.dll", "factory", factory);
    IUnknown_Release(factory);
}

static void test_d2d1(void)
{
    create_d2d_func create;
    IUnknown *factory = (IUnknown *)(ULONG_PTR)1;
    HMODULE module = LoadLibraryA("d2d1.dll");
    HRESULT hr;

    if (!module || !(create = (create_d2d_func)GetProcAddress(module, "D2D1CreateFactory")))
    {
        report("d2d1.dll", "factory", "SKIP", "d2d1.dll or D2D1CreateFactory missing");
        return;
    }

    hr = create(0, &iid_unknown_probe, NULL, (void **)&factory);
    check("d2d1.dll", "factory-bad-iid", FAILED(hr) && !factory, "hr=%#lx out=%p", hr, factory);

    factory = NULL;
    hr = create(0, &iid_d2d_factory, NULL, (void **)&factory);
    if (FAILED(hr))
    {
        report("d2d1.dll", "factory", "FAIL", "hr=%#lx", hr);
        return;
    }
    test_unknown_contract("d2d1.dll", "factory", factory);
    IUnknown_Release(factory);
}

static void test_d3d12(void)
{
    create_d3d12_func create;
    IUnknown *device = (IUnknown *)(ULONG_PTR)1;
    HMODULE module = LoadLibraryA("d3d12.dll");
    D3D12_FEATURE_DATA_D3D12_OPTIONS options;
    HRESULT hr;

    if (!module || !(create = (create_d3d12_func)GetProcAddress(module, "D3D12CreateDevice")))
    {
        report("d3d12.dll", "device", "SKIP", "d3d12.dll or D3D12CreateDevice missing");
        return;
    }

    hr = create(NULL, 0x1, &iid_d3d12_device, (void **)&device);
    check("d3d12.dll", "device-bad-level", FAILED(hr) && !device, "hr=%#lx out=%p", hr, device);

    device = NULL;
    hr = create(NULL, D3D_FEATURE_LEVEL_11_0, &iid_d3d12_device, (void **)&device);
    if (FAILED(hr))
    {
        report("d3d12.dll", "device", "SKIP", "no device at 11_0, hr=%#lx (not a contract result)", hr);
        return;
    }

    test_unknown_contract("d3d12.dll", "device", device);
    hr = ID3D12Device_CheckFeatureSupport((ID3D12Device *)device, D3D12_FEATURE_D3D12_OPTIONS, &options,
            sizeof(options) - 1);
    check("d3d12.dll", "options-bad-size", hr == E_INVALIDARG, "hr=%#lx", hr);
    hr = ID3D12Device_CheckFeatureSupport((ID3D12Device *)device, (D3D12_FEATURE)0x7fff, &options, sizeof(options));
    check("d3d12.dll", "options-unknown-feature", FAILED(hr), "hr=%#lx", hr);
    IUnknown_Release(device);
}

static void test_d3d11(void)
{
    create_d3d11_func create;
    void *device = (void *)(ULONG_PTR)1;
    HMODULE module = LoadLibraryA("d3d11.dll");
    HRESULT hr;

    if (!module || !(create = (create_d3d11_func)GetProcAddress(module, "D3D11CreateDevice")))
    {
        report("d3d11.dll", "device", "SKIP", "d3d11.dll or D3D11CreateDevice missing");
        return;
    }

    hr = create(NULL, 99, NULL, 0, NULL, 0, 7, &device, NULL, NULL);
    check("d3d11.dll", "device-bad-driver-type", FAILED(hr) && !device, "hr=%#lx out=%p", hr, device);
}

static void test_xinput(void)
{
    typedef DWORD (WINAPI *get_state_func)(DWORD, void *);
    get_state_func get_state;
    HMODULE module = LoadLibraryA("xinput1_4.dll");
    BYTE state[16];
    DWORD ret;

    if (!module || !(get_state = (get_state_func)GetProcAddress(module, "XInputGetState")))
    {
        report("xinput1_4.dll", "get-state", "SKIP", "xinput1_4.dll or XInputGetState missing");
        return;
    }
    ret = get_state(99, state);
    check("xinput1_4.dll", "get-state-bad-index", ret != ERROR_SUCCESS, "ret=%lu", ret);
}

int main(void)
{
    printf("CONTRACT-SUITE v%d begin\n", CONTRACT_VERSION);
    test_exports();
    test_dxgi();
    test_dwrite();
    test_d2d1();
    test_d3d12();
    test_d3d11();
    test_xinput();
    printf("CONTRACT-SUITE v%d end pass=%d fail=%d skip=%d\n", CONTRACT_VERSION, pass_count, fail_count, skip_count);
    return fail_count ? 1 : 0;
}
