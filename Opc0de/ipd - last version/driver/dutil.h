/*
 * Copyright (C) 2000-2003 by Pedestal Software, Inc.
 * All Rights Reserved. Use at your own risk!
 * Email: support@pedestalsoftware.com
 *
 * This code is provided as Open Source with restrictions. See the readme
 * file that came with the distribution for details.
 *
 */

/* dutil.h */

#ifndef DUTIL_H
#define DUTIL_H

void dumpbuf(void *buf, int size);
void say_process_mask(ACCESS_MASK mask);
int dumpOA(WCHAR *tag, POBJECT_ATTRIBUTES OA);
int dumpCreateOptions(WCHAR *tag, ULONG opt);

#endif
