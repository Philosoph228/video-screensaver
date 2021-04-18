#ifndef __mfisvc_h__
#define __mfisvc_h__

#include <mfidl.h>

#ifndef __IMFGetService_FWD_DEFINED__
#define __IMFGetService_FWD_DEFINED__
typedef interface IMFGetService IMFGetService;
#ifdef __cplusplus
interface IMFGetService;
#endif /* __cplusplus */
#endif

/*
 *  IMFGetService interface
 */
#ifndef __IMFGetService_INTERFACE_DEFINED__
#define __IMFGetService_INTERFACE_DEFINED__

/* EXTERN_C const IID IID_IMFGetService; */
DEFINE_GUID(IID_IMFGetService, 0xfa993888, 0x4383, 0x415a, 0xa9,0x30,0xdd,0x47,0x2a,0x8c,0xf6,0xf7);
#if defined(__cplusplus) && !defined(CINTERFACE)
MIDL_INTERFACE("fa993888-4383-415a-a930-dd472a8cf6f7")
IMFGetService : public IUnknows
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetService(
        REFGUID guidService,
        REFIID riid,
        LPVOID *ppvObject) = 0;

);
#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(IMFGetService, 0xfa993888, 0x4383, 0x415a, 0xa9,0x30,0xdd,0x47,0x2a,0x8c,0xf6,0xf7)
#endif
#else
typedef struct IMFGetServiceVtbl {
    BEGIN_INTERFACE

    /*** IUnknown methods ***/
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        IMFGetService *This,
        REFIID riid,
        void **ppvObject);

    ULONG (STDMETHODCALLTYPE *AddRef)(
        IMFGetService *This);

    ULONG (STDMETHODCALLTYPE *Release)(
        IMFGetService *This);

    /*** IMFClockStateSink methods ***/
    HRESULT (STDMETHODCALLTYPE *GetService)(
        IMFGetService *This,
        REFGUID guidService,
        REFIID riid,
        LPVOID *ppObject);

    END_INTERFACE
} IMFGetServiceVtbl;

interface IMFGetService {
    CONST_VTBL struct IMFGetServiceVtbl* lpVtbl;
};

#ifdef COBJMACROS
#ifndef WIDL_C_INLINE_WRAPPERS
/*** IUnknown methods ***/
#define IMFGetService_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IMFGetService_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IMFGetService_Release(This) (This)->lpVtbl->Release(This)
/*** IMFGetService methods ***/
#define IMFGetService_GetService(This,guidService,riid,ppvObject) (This)->lpVtbl->GetService(This,guidService,riid,ppvObject)
#else
/*** IUnknown methods ***/
static FORCEINLINE HRESULT IMFGetService_QueryInterface(IMFGetService* This,REFIID riid,void **ppvObject) {
  return This->lpVtbl->QueryInterface(This,riid,ppvObject);
}
static FORCEINLINE ULONG IMFGetService_AddRef(IMFGetService* This) {
  return This->lpVtbl->AddRef(This);
}
static FORCEINLINE ULONG IMFGetService_Release(IMFGetService* This) {
  return This->lpVtbl->Release(This);
}
/*** IMFGetService methods ***/
static FORCEINLINE HRESULT IMFGetService_GetService(IMFGetService* This, REFGUID guidService, REFIID riid, LPVOID* ppObject) {
  return This->lpVtbl->GetService(This,guidService,riid,ppObject);
}
#endif
#endif

#endif


#endif  /* __IMFGetService_INTERFACE_DEFINED__ */

#endif  /* __mfisvc_h__ */
