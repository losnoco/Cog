//
//  mo3.h
//  Dumb MO3 Archive parser
//
//  Created by Christopher Snowhill on 11/1/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#ifdef __cplusplus
extern "C" {
#endif
extern void* unpackMo3(const void* in, long* size);
extern void freeMo3(void* in);
#ifdef __cplusplus
};
#endif
