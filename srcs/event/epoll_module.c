
/*
あとで呼び出しをifdefでわける
 #ifdef __linux__

// linux の場合はepollを使う

#endif

#ifdef __MACH__
// Mach の場合はkqueueを使う
#endif

// pollはportabilityあるが　スケールしない */

// level triggerd ... notify the readiness of a fd
// edge triggered