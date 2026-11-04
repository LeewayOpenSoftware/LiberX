#define COBJMACROS
#define CONST_VTABLE
#define INITGUID
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <dwrite.h>

#define TEST_VERSION 1
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static int fail_count, skip_count, pass_count;

static void report(const char *name, const char *status, const char *fmt, ...)
{
    char detail[256];
    va_list args;

    va_start(args, fmt);
    vsnprintf(detail, sizeof(detail), fmt, args);
    va_end(args);
    printf("DWRITE_HITTEST v%d %s %s %s\n", TEST_VERSION, name, status, detail);
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

typedef HRESULT (WINAPI *create_factory_func)(DWRITE_FACTORY_TYPE, REFIID, IUnknown **);

static void test_hittestpoint_basic(IDWriteFactory *factory)
{
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    DWRITE_HIT_TEST_METRICS metrics;
    BOOL is_trailing, is_inside;
    const WCHAR text[] = L"Hello";
    HRESULT hr;

    hr = IDWriteFactory_CreateTextFormat(factory, L"Arial", NULL, DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-us", &format);
    if (FAILED(hr)) {
        report("hittestpoint_basic", "SKIP", "CreateTextFormat failed");
        return;
    }

    hr = IDWriteFactory_CreateTextLayout(factory, text, ARRAY_SIZE(text) - 1, format, 200.0f, 100.0f, &layout);
    IDWriteTextFormat_Release(format);
    if (FAILED(hr)) {
        report("hittestpoint_basic", "SKIP", "CreateTextLayout failed");
        return;
    }

    hr = IDWriteTextLayout_HitTestPoint(layout, 5.0f, 5.0f, &is_trailing, &is_inside, &metrics);
    check("hittestpoint_basic", SUCCEEDED(hr) && is_inside && metrics.textPosition == 0,
            "first char: hr=%lx inside=%d pos=%u", hr, is_inside, metrics.textPosition);

    hr = IDWriteTextLayout_HitTestPoint(layout, 500.0f, 500.0f, &is_trailing, &is_inside, &metrics);
    check("hittestpoint_outside", SUCCEEDED(hr) && !is_inside,
            "outside: hr=%lx inside=%d", hr, is_inside);

    IDWriteTextLayout_Release(layout);
}

static void test_hittesttextrange_basic(IDWriteFactory *factory)
{
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    DWRITE_HIT_TEST_METRICS metrics_array[10];
    UINT32 actual_count;
    const WCHAR text[] = L"Hello";
    HRESULT hr;

    hr = IDWriteFactory_CreateTextFormat(factory, L"Arial", NULL, DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-us", &format);
    if (FAILED(hr)) {
        report("hittesttextrange_basic", "SKIP", "CreateTextFormat failed");
        return;
    }

    hr = IDWriteFactory_CreateTextLayout(factory, text, ARRAY_SIZE(text) - 1, format, 200.0f, 100.0f, &layout);
    IDWriteTextFormat_Release(format);
    if (FAILED(hr)) {
        report("hittesttextrange_basic", "SKIP", "CreateTextLayout failed");
        return;
    }

    hr = IDWriteTextLayout_HitTestTextRange(layout, 0, 5, 0.0f, 0.0f, metrics_array, ARRAY_SIZE(metrics_array), &actual_count);
    check("hittesttextrange_full", SUCCEEDED(hr) && actual_count >= 1,
            "full range: hr=%lx count=%u", hr, actual_count);

    hr = IDWriteTextLayout_HitTestTextRange(layout, 1, 2, 0.0f, 0.0f, metrics_array, ARRAY_SIZE(metrics_array), &actual_count);
    check("hittesttextrange_partial", SUCCEEDED(hr) && actual_count >= 1,
            "partial: hr=%lx count=%u", hr, actual_count);

    IDWriteTextLayout_Release(layout);
}

static void test_dwritecreatfactory_invalid_type(void)
{
    HMODULE dwrite;
    create_factory_func create_factory;
    IDWriteFactory *factory;
    HRESULT hr;

    dwrite = LoadLibraryA("dwrite.dll");
    if (!dwrite) {
        report("factory_invalid_type", "SKIP", "dwrite.dll not found");
        return;
    }

    create_factory = (create_factory_func)GetProcAddress(dwrite, "DWriteCreateFactory");
    if (!create_factory) {
        report("factory_invalid_type", "SKIP", "DWriteCreateFactory export");
        FreeLibrary(dwrite);
        return;
    }

    factory = (IDWriteFactory *)0xdeadbeef;
    hr = create_factory(999, &IID_IDWriteFactory, (IUnknown **)&factory);
    check("factory_invalid_type", hr == E_INVALIDARG && factory == NULL,
            "invalid type: hr=%lx factory=%p", hr, factory);

    FreeLibrary(dwrite);
}

int main(void)
{
    create_factory_func create_factory;
    IDWriteFactory *factory;
    HMODULE dwrite;
    HRESULT hr;

    dwrite = LoadLibraryA("dwrite.dll");
    if (!dwrite) {
        printf("DWRITE_HITTEST v%d LOAD FAIL dwrite.dll not found\n", TEST_VERSION);
        return 1;
    }

    create_factory = (create_factory_func)GetProcAddress(dwrite, "DWriteCreateFactory");
    if (!create_factory) {
        printf("DWRITE_HITTEST v%d LOAD FAIL DWriteCreateFactory export\n", TEST_VERSION);
        FreeLibrary(dwrite);
        return 1;
    }

    hr = create_factory(DWRITE_FACTORY_TYPE_SHARED, &IID_IDWriteFactory, (IUnknown **)&factory);
    if (FAILED(hr)) {
        printf("DWRITE_HITTEST v%d LOAD FAIL DWriteCreateFactory hr=%lx\n", TEST_VERSION, hr);
        FreeLibrary(dwrite);
        return 1;
    }

    test_dwritecreatfactory_invalid_type();

    test_hittestpoint_basic(factory);
    test_hittesttextrange_basic(factory);

    IDWriteFactory_Release(factory);
    FreeLibrary(dwrite);

    printf("DWRITE_HITTEST v%d SUMMARY PASS=%d FAIL=%d SKIP=%d\n", TEST_VERSION, pass_count, fail_count, skip_count);
    return fail_count;
}
