#pragma once

#ifdef VMPROTECT
#include "../VMProtect/Include/C/VMProtectDDK.h"

#define VMP_STRINGIFY_(x) #x
#define VMP_STRINGIFY(x) VMP_STRINGIFY_(x)
#define VMP_DEFAULT_LABEL __FILE__ ":" __FUNCTION__ "(" VMP_STRINGIFY(__LINE__) ")"

#define VMP_BEGIN() VMProtectBegin(VMP_DEFAULT_LABEL);
#define VMP_BEGINVIRTUALIZATION() VMProtectBeginVirtualization(VMP_DEFAULT_LABEL);
#define VMP_BEGINMUTATION() VMProtectBeginMutation(VMP_DEFAULT_LABEL);
#define VMP_BEGINULTRA() VMProtectBeginUltra(VMP_DEFAULT_LABEL);
#define VMP_BEGINVIRTUALIZATIONLOCKBYKEY() VMProtectBeginVirtualizationLockByKey(VMP_DEFAULT_LABEL);
#define VMP_BEGINULTRALOCKBYKEY() VMProtectBeginUltraLockByKey(VMP_DEFAULT_LABEL);
#define VMP_END() VMProtectEnd()

#define VMP_ISPROTECTED VMProtectIsProtected()
#define VMP_ISDEBUGGERPRESENT(bcheckkernel) VMProtectIsDebuggerPresent(bcheckkernel)
#define VMP_ISVIRTUALMACHINEPRESENT() VMProtectIsVirtualMachinePresent()
#define VMP_ISVALIDIMAGECRC() VMProtectIsValidImageCRC()
#define VMP_ENCDECSTRINGA(value) VMProtectDecryptStringA(value)
#define VMP_ENCDECSTRINGW(value) VMProtectDecryptStringW(value)
#define VMP_FREESTRING(value) VMProtectFreeString(value)
#else
#define VMP_BEGIN()
#define VMP_BEGINVIRTUALIZATION()
#define VMP_BEGINMUTATION()
#define VMP_BEGINULTRA()
#define VMP_BEGINVIRTUALIZATIONLOCKBYKEY()
#define VMP_BEGINULTRALOCKBYKEY()
#define VMP_END()

#define VMP_ISPROTECTED false
#define VMP_ISDEBUGGERPRESENT(bcheckkernel) false
#define VMP_ISVIRTUALMACHINEPRESENT() false
#define VMP_ISVALIDIMAGECRC true
#define VMP_ENCDECSTRINGA(value) value
#define VMP_ENCDECSTRINGW(value) value
#define VMP_FREESTRING(value) true
#endif