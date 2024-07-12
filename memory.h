#pragma once

void EnumeratePages(void (*pageCallback) (void* address, int size));
bool IsAddressValid(const void* address);