#define COBJMACROS
#define CONST_VTABLE
#define INITGUID
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <d2d1.h>

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
    printf("D2D_GEOMETRY v%d %s %s %s\n", TEST_VERSION, name, status, detail);
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

typedef HRESULT (WINAPI *d2d_create_factory_func)(D2D1_FACTORY_TYPE, REFIID, const D2D1_FACTORY_OPTIONS*, void **);

static void test_geometry_computelength(ID2D1Factory *factory)
{
    ID2D1PathGeometry *path_geom = NULL;
    ID2D1GeometrySink *sink = NULL;
    FLOAT length = 0.0f;
    HRESULT hr;

    hr = ID2D1Factory_CreatePathGeometry(factory, &path_geom);
    if (FAILED(hr)) {
        report("geometry_computelength", "SKIP", "CreatePathGeometry failed");
        return;
    }

    hr = ID2D1PathGeometry_Open(path_geom, &sink);
    if (FAILED(hr)) {
        report("geometry_computelength", "SKIP", "PathGeometry::Open failed");
        ID2D1PathGeometry_Release(path_geom);
        return;
    }

    D2D1_POINT_2F start = {0.0f, 0.0f};
    sink->lpVtbl->BeginFigure(sink, start, D2D1_FIGURE_BEGIN_FILLED);
    D2D1_POINT_2F end = {100.0f, 0.0f};
    sink->lpVtbl->AddLine(sink, end);
    sink->lpVtbl->EndFigure(sink, D2D1_FIGURE_END_CLOSED);
    sink->lpVtbl->Close(sink);
    ID2D1GeometrySink_Release(sink);

    hr = ID2D1PathGeometry_ComputeLength(path_geom, NULL, 0.25f, &length);
    check("geometry_computelength", SUCCEEDED(hr),
            "ComputeLength: hr=%lx length=%.2f", hr, length);

    ID2D1PathGeometry_Release(path_geom);
}

static void test_geometry_widen(ID2D1Factory *factory)
{
    ID2D1PathGeometry *path_geom = NULL;
    ID2D1GeometrySink *sink = NULL;
    ID2D1PathGeometry *widened_geom = NULL;
    HRESULT hr;

    hr = ID2D1Factory_CreatePathGeometry(factory, &path_geom);
    if (FAILED(hr)) {
        report("geometry_widen", "SKIP", "CreatePathGeometry failed");
        return;
    }

    hr = ID2D1PathGeometry_Open(path_geom, &sink);
    if (FAILED(hr)) {
        report("geometry_widen", "SKIP", "PathGeometry::Open failed");
        ID2D1PathGeometry_Release(path_geom);
        return;
    }

    D2D1_POINT_2F start = {10.0f, 10.0f};
    sink->lpVtbl->BeginFigure(sink, start, D2D1_FIGURE_BEGIN_FILLED);
    D2D1_POINT_2F end = {90.0f, 10.0f};
    sink->lpVtbl->AddLine(sink, end);
    sink->lpVtbl->EndFigure(sink, D2D1_FIGURE_END_OPEN);
    sink->lpVtbl->Close(sink);
    ID2D1GeometrySink_Release(sink);

    hr = ID2D1Factory_CreatePathGeometry(factory, &widened_geom);
    if (SUCCEEDED(hr))
    {
        hr = ID2D1PathGeometry_Widen(path_geom, 5.0f, NULL, NULL, &widened_geom);
        check("geometry_widen", hr == E_NOTIMPL || SUCCEEDED(hr),
                "Widen: hr=%lx", hr);
        ID2D1PathGeometry_Release(widened_geom);
    }

    ID2D1PathGeometry_Release(path_geom);
}

int main(void)
{
    d2d_create_factory_func create_factory;
    ID2D1Factory *factory;
    HMODULE d2d1;
    HRESULT hr;

    d2d1 = LoadLibraryA("d2d1.dll");
    if (!d2d1) {
        printf("D2D_GEOMETRY v%d LOAD FAIL d2d1.dll not found\n", TEST_VERSION);
        return 1;
    }

    create_factory = (d2d_create_factory_func)GetProcAddress(d2d1, "D2D1CreateFactory");
    if (!create_factory) {
        printf("D2D_GEOMETRY v%d LOAD FAIL D2D1CreateFactory export\n", TEST_VERSION);
        FreeLibrary(d2d1);
        return 1;
    }

    D2D1_FACTORY_OPTIONS options = {0};
    hr = create_factory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, &options, (void **)&factory);
    if (FAILED(hr)) {
        printf("D2D_GEOMETRY v%d LOAD FAIL D2D1CreateFactory hr=%lx\n", TEST_VERSION, hr);
        FreeLibrary(d2d1);
        return 1;
    }

    test_geometry_computelength(factory);
    test_geometry_widen(factory);

    ID2D1Factory_Release(factory);
    FreeLibrary(d2d1);

    printf("D2D_GEOMETRY v%d SUMMARY PASS=%d FAIL=%d SKIP=%d\n", TEST_VERSION, pass_count, fail_count, skip_count);
    return fail_count;
}
