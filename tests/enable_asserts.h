#pragma once
// Force executable regression assertions active even with MSVC Release /DNDEBUG.
#ifdef NDEBUG
#undef NDEBUG
#endif
