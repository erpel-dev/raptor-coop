#pragma once

/* Returns 1 if required shareware/full game data is present. */
int RAP_HasGameData(void);

/*
 * Offers to download the shareware GLB files from DOS Games Archive.
 * Returns 1 if data is available afterwards, 0 if the user cancelled or
 * the download/extract failed.
 */
int RAP_OfferAssetDownload(void);
