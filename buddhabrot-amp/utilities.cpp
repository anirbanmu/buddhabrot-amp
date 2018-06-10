#include "utilities.h"

void throw_hresult_on_failure(HRESULT hr)
{
    if (FAILED(hr)) throw hr;
}
