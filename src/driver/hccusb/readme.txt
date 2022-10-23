USB driver for AT91SAM7 devices
================================================
Maj 7, 2006
--------------------------------

Unsupported features:
    The driver does not supports isochronous endpoints.

The driver implements the following standard requests.
    Requests to the device:
        Set address
        Get descriptor / device descriptor
        Get descriptor / configuration descriptor
        Get descriptor / string descriptor
        Get configuration
        Set configuration
    Requests to an interface:
        None
    Requests to an endpoint:
        Clear feature / Feature endpoint halt

    If the device receives a not implemented request, then a user callback function
    of endpoint 0 will be called. This makes it possible to extend the driver and
    implement the missing requests.

Interrupts
    The driver is interrupt driven.
    The USB module will request an interrupt for the following events:
        Stall sent.
        Frame received ok.
        Frame transmitted ok.
        USB BUS suspend.
        USB BUS wakeup.
        USB BUS reset.

    The driver will not disable interupts at core level any time.

USB configuration rules
   These rules apply when creating the configuratin descriptor.
   - For interfaces having alternate settings, the first interface descriptor
     shall be the default (alternate setting 0).
   - Interfaces must be numbered from 0 increasing continously (0,1,2,3...).
   - Configurations must be numbered from 1 increasing continously.
   - Endpoints will have a fixed address that equals to their index. This means
     the endpoint described by the first endpoint descriptor wil have the address
     1 or 0x81, the second 2 or 0x82, the thrid 3 or 0x83. In endpoints have
     bit 7 of their address set to one.
   
   Endpoints get numbered as their descriptors are found in the configuration
   descriptor. For example if you have two configurations each having one
   intarface and two endpoints. When configuration one is active the endpoint
   described by the first descriptor will have index one and the second one
   index two. If configuration 2 is active, the same happens and endpoint
   described by the first descriptor will have index one and the second one
   index two. Endpoint index three and four will never be valid. You have to use
   these indexes as parameters to API calls where required.

USB configuration MACROS
   IT_ROUTINE_IS_ISR
     Define with value 1, if the interrupt routine is called as an interrupt
     service routine. This is the case, if the IRQ service routine at 0x18 will
     directly jump to this routine without executing any interrupt prologue code
     and the interrupt routine will directly return to the interrupted code.

     This means that the usb_it_handler needs a special prologue and epilogue,
     and interrupt flag needs to be cleared in AIC. The default IAR (cstartup.
     s79) and GNU (crt0.s) startup code will not work this way     (
     IT_ROUTINE_IS_ISR shall be 0). If you use a custom startup file, you may
     set IT_ROUTINE_IS_ISR to one.

   NDEBUG
     Will remove debugging related code (HCC_ASSERT loops, debug trace
     buffer, etc...)

======= END OF FILE ===========================================================


