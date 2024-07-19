/* This file is part of Racdoor.
   Copyright (c) 2024 chaoticgd. All rights reversed.
   Released under the BSD-1-Clause license. */

#ifndef _GAME_KLIB_H
#define _GAME_KLIB_H

struct ThreadParam;
struct SemaParam;
struct SifDmaTransfer;

/* Parameters taken from https://psi-rockin.github.io/ps2tek/. */

void RFU000_FullReset(); /* 0x0 */
void ResetEE(int reset_flag); /* 0x1 */
void SetGsCrt(bool interlaced, int display_mode, bool frame); /* 0x2 */
void RFU003(); /* 0x3 */
void _Exit(); /* 0x4 */
void RFU005(const char* filename, int argc, char** argv); /* 0x5 _ExceptionEpilogue */
void _LoadExecPS2(const char* filename, int argc, char** argv); /* 0x6 */
void _ExecPS2(void* entry, void* gp, int argc, char** argv); /* 0x7 */
void RFU008(); /* 0x8 */
void RFU009(); /* 0x9 */
void AddSbusIntcHandler(); /* 0xa */
void RemoveSbusIntcHandler(); /* 0xb */
void Interrupt2Iop(); /* 0xc */
void SetVTLBRefillHandler(); /* 0xd */
void SetVCommonHandler(); /* 0xe */
void SetVInterruptHandler(); /* 0xf */
int AddIntcHandler(int int_cause, int (*handler)(int), int next, void* arg, int flag); /* 0x10 */
int AddIntcHandler2(int int_cause, int (*handler)(int), int next, void* arg, int flag); /* 0x10 */
int RemoveIntcHandler(int int_cause, int handler_id); /* 0x11 */
int AddDmacHandler(int dma_cause, int (*handler)(int), int next, void* arg, int flag); /* 0x12 */
int AddDmacHandler2(int dma_cause, int (*handler)(int), int next, void* arg, int flag); /* 0x12 */
int RemoveDmacHandler(int dma_cause, int handler_id); /* 0x13 */
int _EnableIntc(int cause_bit); /* 0x14 */
int _DisableIntc(int cause_bit); /* 0x15 */
int _EnableDmac(int cause_bit); /* 0x16  */
int _DisableDmac(); /* 0x17 */
void SetAlarm(); /* 0xfc */
void ReleaseAlarm(); /* 0xfd */
void _iEnableIntc(int cause_bit); /* -0x1a */
void _iDisableIntc(int cause_bit); /* -0x1b */
void _iEnableDmac(int cause_bit); /* -0x1c */
void _iDisableDmac(int cause_bit); /* -0x1d */
void iSetAlarm(); /* -0xfe */
void iReleaseAlarm(); /* -0xff */
int CreateThread(ThreadParam* t); /* 0x20 */
void DeleteThread(int thread_id); /* 0x21 */
void _StartThread(int thread_id, void* arg); /* 0x22 */
void ExitThread(); /* 0x23 */
void ExitDeleteThread(); /* 0x24 */
void TerminateThread(int thread_id); /* 0x25 */
void iTerminateThread(int thread_id); /* -0x26 */
void DisableDispatchThread(); /* 0x27 */
void EnableDispatchThread(); /* 0x28 */
int ChangeThreadPriority(int thread_id, int priority); /* 0x29 */
int iChangeThreadPriority(int thread_id, int priority); /* -0x2a */
void RotateThreadReadyQueue(int priority); /* 0x2b */
int _iRotateThreadReadyQueue(int priority); /* -0x2c */
void ReleaseWaitThread(int thread_id); /* 0x2d */
int iReleaseWaitThread(int thread_id); /* -0x2e */
int GetThreadId(); /* 0x2f */
int ReferThreadStatus(int thread_id, ThreadParam* status); /* 0x30 */
int iReferThreadStatus(int thread_id, ThreadParam* status); /* -0x31 */
void SleepThread(); /* 0x32 */
void WakeupThread(int thread_id); /* 0x33 */
void _iWakeupThread(int thread_id); /* -0x34 */
int CancelWakeupThread(int thread_id); /* 0x35 */
int iCancelWakeupThread(int thread_id); /* -0x36 */
int SuspendThread(int thread_id); /* 0x37 */
int _iSuspendThread(int thread_id); /* -0x38 */
void ResumeThread(int thread_id); /* 0x39 */
int iResumeThread(int thread_id); /* -0x3a */
void JoinThread(); /* 0x3b */
void* RFU060(unsigned int gp, void* stack, int stack_size, char* args, int root); /* 0x3c InitMainThread */
void* RFU061(void* heap, int heap_size); /* 0x3d InitHeap */
void* EndOfHeap(); /* 0x3e */
void RFU063(); /* 0x3f */
int CreateSema(SemaParam* s); /* 0x40 */
int DeleteSema(int sema_id); /* 0x41 */
int SignalSema(int sema_id); /* 0x42 */
int iSignalSema(int sema_id); /* 0x43 */
void WaitSema(int sema_id); /* 0x44 */
int PollSema(int sema_id); /* 0x45 */
int iPollSema(int sema_id); /* -0x46 */
void ReferSemaStatus(); /* 0x47 */
void iReferSemaStatus(); /* -0x48 */
void RFU073(); /* 0x49 */
void SetOsdConfigParam(); /* 0x4a */
void GetOsdConfigParam(); /* 0x4b */
void GetGsHParam(); /* 0x4c */
void GetGsVParam(); /* 0x4d */
void SetGsHParam(); /* 0x4e */
void SetGsVParam(); /* 0x4f */
void RFU080_CreateEventFlag(); /* 0x50 */
void RFU081_DeleteEventFlag(); /* 0x51 */
void RFU082_SetEventFlag(); /* 0x52 */
void RFU083_iSetEventFlag(); /* -0x53 */
void RFU084_ClearEventFlag(); /* 0x54 */
void RFU085_iClearEventFlag(); /* -0x55 */
void RFU086_WaitEvnetFlag(); /* 0x56 */
void RFU087_PollEvnetFlag(); /* 0x57 */
void RFU088_iPollEvnetFlag(); /* -0x58 */
void RFU089_ReferEventFlagStatus(); /* 0x59 */
void RFU090_iReferEventFlagStatus(); /* -0x5a */
void RFU091(); /* 0x5b */
void EnableIntcHandler(); /* 0x5c */
void iEnableIntcHandler(); /* -0x5c */
void DisableIntcHandler(); /* 0x5d */
void iDisableIntcHandler(); /* -0x5d */
void EnableDmacHandler(); /* 0x5e */
void iEnableDmacHandler(); /* -0x5e */
void DisableDmacHandler(); /* 0x5f */
void iDisableDmacHandler(); /* -0x5f */
void KSeg0(); /* 0x60 */
void EnableCache(); /* 0x61 */
void DisableCache(); /* 0x62 */
void GetCop0(); /* 0x63 */
void FlushCache(int mode); /* 0x64 */
void CpuConfig(); /* 0x66 */
void iGetCop0(); /* -0x67 */
void iFlushCache(); /* -0x68 */
void iCpuConfig(); /* -0x6a */
void sceSifStopDma(); /* 0x6b */
void SetCPUTimerHandler(); /* 0x6c */
void SetCPUTimer(); /* 0x6d */
void SetOsdConfigParam2(); /* 0x6e */
void GetOsdConfigParam2(); /* 0x6f */
unsigned long GsGetIMR(); /* 0x70 */
unsigned long iGsGetIMR(); /* -0x70 */
void GsPutIMR(unsigned long value); /* 0x71 */
void iGsPutIMR(unsigned long value); /* -0x71 */
void SetPgifHandler(); /* 0x72 */
void SetVSyncFlag(int* vsync_occurred, unsigned long* csr_stat_on_vsync); /* 0x73 */
void RFU116(int index, int address); /* 0x74 SetSyscall */
void _print(); /* 0x75 */
int sceSifDmaStat(unsigned int dma_id); /* 0x76 */
int isceSifDmaStat(); /* -0x76 */
unsigned int sceSifSetDma(SifDmaTransfer* trans, int len); /* 0x77 */
void isceSifSetDma(); /* -0x77 */
void sceSifSetDChain(); /* 0x78 */
void isceSifSetDChain(); /* -0x78 */
void sceSifSetReg(); /* 0x79 */
void sceSifGetReg(); /* 0x7a */
void _ExecOSD(int argc, char** argv); /* 0x7b */
void Deci2Call(); /* 0x7c */
void PSMode(); /* 0x7d */
int MachineType(); /* 0x7e */
int GetMemorySize(); /* 0x7f */
void _InitTLB(); /* 0x82 */

#endif
