#pragma once

#include <stdint.h>

#include "app/page/page_catalog.h"
#include "app/page/page_manager.h"

PageId sanitizeStoredPageId(int32_t rawPageId, const PageSettings &settings);
PageId loadPersistedCurrentPage(const PageSettings &settings);
bool savePersistedCurrentPage(PageId page);
