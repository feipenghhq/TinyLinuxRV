# Coremark

> From https://github.com/eembc/coremark
>
> CoreMark's primary goals are simplicity and providing a method for testing only a processor's core features. For more information about EEMBC's comprehensive embedded benchmark suites, please see www.eembc.org.

> For a more compute-intensive version of CoreMark that uses larger datasets and execution loops taken from common applications, please check out EEMBC's CoreMark-PRO benchmark, also on GitHub.

## Tinylinuxrv

tinylinuxrv ported the `coremark/barebones` to tinylinuxrv baremetal environment.

Check [barebones_porting.md](../../../../third-party/coremark/barebones_porting.md) for more details about porting requirement/methods.

Here are all the changes made by tinylinuxrv

### core_portme.h

> all the changes has a comment of "tinylinuxrv"

- C macro update

| Preprocessing macro | Value | comment                   |
| ------------------- | ----- | ------------------------- |
| HAS_FLOAT           | 0     | No floating point support |
| HAS_TIME_H          | 0     | No time.h                 |
| USE_CLOCK           | 0     | No time.h                 |
| HAS_STDIO           | 0     | No stdio.h                |
| HAS_PRINTF          | 0     | No stdio.h                |
| MAIN_HAS_NOARGC     | 1     | No argument for main      |
| MAIN_HAS_NORETURN   | 0     | has return value for main |
