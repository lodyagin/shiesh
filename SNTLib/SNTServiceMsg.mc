;#ifndef __SNTSERVICEMSG_H
;#define __SNTSERVICEMSG_H
;

 
MessageIdTypedef=DWORD

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0:FACILITY_SYSTEM
               Runtime=0x2:FACILITY_RUNTIME
               Stubs=0x3:FACILITY_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
              )

LanguageNames=(English=0x409:MSG00409)
LanguageNames=(Russian=0x419:MSG00419)


MessageId=0x1
SymbolicName=MSG_START_SRV_ERROR
Facility=Runtime
Severity=Error
Language=English
Can not start service: %1.
.
Language=Russian
Невозможно стартовать сервис: %1.
.


;
;#endif // __SNTSERVICEMSG_H
;
