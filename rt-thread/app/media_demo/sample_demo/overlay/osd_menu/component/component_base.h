#ifndef _COMPONENT_BASE_H_
#define _COMPONENT_BASE_H_

// virtual function
typedef void (*fGetValueCallback)(void *self);
typedef void (*fDataChangeCallback)(void *self);
typedef void (*fClickCallback)(void *self);
typedef void (*fDrawCallback)(void *self);

#endif  // !_OSD_COMPONENT_BASE_H_