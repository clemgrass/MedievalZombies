#pragma once

// Conversión implícita de long a int: un warning con -Wconversion.
inline int noisy_narrow(long value) { return value; }

int noisy_sum(long a, long b);
