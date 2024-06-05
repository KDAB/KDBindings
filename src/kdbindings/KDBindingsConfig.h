#pragma once

#ifdef KDBINDINGS_ENABLE_WARN_UNUSED
#define KDBINDINGS_WARN_UNUSED [[nodiscard]]
#else
#define KDBINDINGS_WARN_UNUSED
#endif
