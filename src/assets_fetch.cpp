#include "assets_fetch.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "SDL.h"
#include "rap.h"
#include "loadsave.h"

#include "miniz.h"

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")
#else
#include <unistd.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* Shareware package hosted by DOS Games Archive (not redistributed with Raptor). */
static const char *kInfoPageUrl =
    "https://www.dosgamesarchive.com/download/raptor-call-of-the-shadows";
static const char *kZipDownloadUrl =
    "https://image.dosgamesarchive.com/games/rapt-box.zip";
/* Alternate redirect used by the archive file.php?id=555 page. */
static const char *kZipDownloadUrlAlt =
    "https://www.dosgamesarchive.com/file.php?id=555";

static int
file_exists_cwd_or_path(const char *upper, const char *lower)
{
    if (!access(upper, 0) || !access(lower, 0))
        return 1;
    if (RAP_CheckFileInPath(upper) || RAP_CheckFileInPath(lower))
        return 1;
    return 0;
}

int
RAP_HasGameData(void)
{
    int loop;
    int numfiles = 0;

    memset(gameflag, 0, sizeof(gameflag));
    reg_flag = 0;

    if (file_exists_cwd_or_path("FILE0001.GLB", "file0001.glb"))
        gameflag[0] = 1;

    if (file_exists_cwd_or_path("FILE0002.GLB", "file0002.glb"))
        gameflag[1] = 1;

    if (file_exists_cwd_or_path("FILE0003.GLB", "file0003.glb") &&
        file_exists_cwd_or_path("FILE0004.GLB", "file0004.glb"))
    {
        gameflag[2] = 1;
        gameflag[3] = 1;
    }

    if (gameflag[1] + gameflag[2])
        reg_flag = 1;

    for (loop = 0; loop < 4; loop++)
    {
        if (gameflag[loop])
            numfiles++;
    }

    if (!file_exists_cwd_or_path("FILE0000.GLB", "file0000.glb"))
        return 0;

    return numfiles > 0;
}

static void
build_path(char *out, size_t out_sz, const char *dir, const char *name)
{
    if (dir && dir[0])
        snprintf(out, out_sz, "%s%s", dir, name);
    else
        snprintf(out, out_sz, "%s", name);
}

static int
download_url_to_file(const char *url, const char *dest)
{
#ifdef _WIN32
    HRESULT hr = URLDownloadToFileA(NULL, url, dest, 0, NULL);
    return SUCCEEDED(hr);
#else
    char cmd[PATH_MAX * 2 + 256];
    int rc;

    /* Prefer curl; fall back to wget. Paths come from our temp/pref dirs. */
    snprintf(cmd, sizeof(cmd),
             "curl -fsSL --connect-timeout 30 --retry 2 -o \"%s\" \"%s\"",
             dest, url);
    rc = system(cmd);
    if (rc == 0)
        return 1;

    snprintf(cmd, sizeof(cmd),
             "wget -q -O \"%s\" \"%s\"",
             dest, url);
    rc = system(cmd);
    return rc == 0;
#endif
}

static int
extract_shareware_glbs(const char *zip_path, const char *dest_dir)
{
    mz_zip_archive zip;
    char out0[PATH_MAX];
    char out1[PATH_MAX];
    mz_bool ok;

    memset(&zip, 0, sizeof(zip));
    if (!mz_zip_reader_init_file(&zip, zip_path, 0))
    {
        printf("assets_fetch: failed to open zip '%s'\n", zip_path);
        return 0;
    }

    build_path(out0, sizeof(out0), dest_dir, "FILE0000.GLB");
    build_path(out1, sizeof(out1), dest_dir, "FILE0001.GLB");

    ok = mz_zip_reader_extract_file_to_file(
        &zip, "RAPTOR/FILE0000.GLB", out0, 0);
    if (!ok)
    {
        /* Some packs may use different casing / layout. */
        ok = mz_zip_reader_extract_file_to_file(
            &zip, "FILE0000.GLB", out0, 0);
    }
    if (!ok)
    {
        printf("assets_fetch: FILE0000.GLB not found in archive\n");
        mz_zip_reader_end(&zip);
        return 0;
    }

    ok = mz_zip_reader_extract_file_to_file(
        &zip, "RAPTOR/FILE0001.GLB", out1, 0);
    if (!ok)
    {
        ok = mz_zip_reader_extract_file_to_file(
            &zip, "FILE0001.GLB", out1, 0);
    }
    if (!ok)
    {
        printf("assets_fetch: FILE0001.GLB not found in archive\n");
        mz_zip_reader_end(&zip);
        return 0;
    }

    mz_zip_reader_end(&zip);
    printf("assets_fetch: installed shareware data to %s\n", dest_dir);
    return 1;
}

static int
prompt_missing_assets(int *button_id)
{
    const SDL_MessageBoxButtonData buttons[] = {
        { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Download shareware" },
        { 0, 1, "View download page" },
        { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 2, "Quit" },
    };
    const SDL_MessageBoxData box = {
        SDL_MESSAGEBOX_WARNING,
        NULL,
        "Raptor — game data missing",
        "Required GLB game data files were not found.\n\n"
        "Raptor does not bundle original assets for licensing reasons.\n"
        "You can download the freeware/shareware data (FILE0000.GLB and "
        "FILE0001.GLB) from DOS Games Archive, or open that page in your "
        "browser and install the files yourself.\n\n"
        "Download location after install:\n"
        "  the Raptor data directory (SDL pref path)",
        SDL_arraysize(buttons),
        buttons,
        NULL
    };

    return SDL_ShowMessageBox(&box, button_id) == 0;
}

static int
download_and_install(void)
{
    const char *dest_dir = RAP_GetPath();
    char zip_path[PATH_MAX];
    char tmp_dir[PATH_MAX];
    int ok = 0;

    if (!dest_dir || !dest_dir[0])
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Raptor",
            "Could not determine the Raptor data directory.", NULL);
        return 0;
    }

    snprintf(tmp_dir, sizeof(tmp_dir), "%s", dest_dir);
    build_path(zip_path, sizeof(zip_path), tmp_dir, "rapt-box.zip");

    printf("assets_fetch: downloading shareware package...\n");
    printf("assets_fetch: %s\n", kZipDownloadUrl);
    fflush(stdout);

    if (!download_url_to_file(kZipDownloadUrl, zip_path))
    {
        printf("assets_fetch: primary URL failed, trying alternate...\n");
        if (!download_url_to_file(kZipDownloadUrlAlt, zip_path))
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Raptor",
                "Download failed.\n\n"
                "Check your network connection, or open the download page "
                "and install FILE0000.GLB / FILE0001.GLB manually.",
                NULL);
            return 0;
        }
    }

    ok = extract_shareware_glbs(zip_path, dest_dir);
    remove(zip_path);

    if (!ok)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Raptor",
            "Download succeeded but extracting the GLB files failed.",
            NULL);
        return 0;
    }

    if (!RAP_HasGameData())
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Raptor",
            "GLB files were written but still could not be validated.",
            NULL);
        return 0;
    }

    if (getenv("RAPTOR_AUTO_FETCH_ASSETS") == NULL)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Raptor",
            "Shareware game data installed successfully.",
            NULL);
    }
    else
    {
        printf("assets_fetch: shareware game data installed successfully.\n");
    }
    return 1;
}

int
RAP_OfferAssetDownload(void)
{
#if defined(__ANDROID__)
    {
        int button_id = 2;
        const SDL_MessageBoxButtonData buttons[] = {
            { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Open download page" },
            { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 2, "Quit" },
        };
        const SDL_MessageBoxData box = {
            SDL_MESSAGEBOX_WARNING,
            NULL,
            "Raptor — game data missing",
            "Required GLB game data files were not found.\n\n"
            "Please copy FILE0000.GLB and FILE0001.GLB (shareware) into the "
            "Raptor data directory, or open the DOS Games Archive page.",
            SDL_arraysize(buttons),
            buttons,
            NULL
        };
        if (SDL_ShowMessageBox(&box, &button_id) == 0 && button_id == 1)
            SDL_OpenURL(kInfoPageUrl);
        return 0;
    }
#else
    /* Non-interactive fetch for scripts/CI: RAPTOR_AUTO_FETCH_ASSETS=1 */
    if (getenv("RAPTOR_AUTO_FETCH_ASSETS") != NULL)
        return download_and_install();

    for (;;)
    {
        int button_id = 2;

        if (!prompt_missing_assets(&button_id))
            return 0;

        if (button_id == 1)
        {
            if (SDL_OpenURL(kInfoPageUrl) != 0)
            {
                printf("assets_fetch: SDL_OpenURL failed: %s\n", SDL_GetError());
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Raptor",
                    kInfoPageUrl, NULL);
            }
            continue;
        }

        if (button_id == 0)
            return download_and_install();

        return 0;
    }
#endif
}
