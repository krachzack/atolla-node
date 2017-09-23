#ifndef MSG_TYPE_H
#define MSG_TYPE_H

enum MsgType
{
    MSG_TYPE_BORROW = 0,
    MSG_TYPE_LENT = 1,
    MSG_TYPE_ENQUEUE = 2,
    MSG_TYPE_FAIL = 255
};
typedef enum MsgType MsgType;

#endif // MSG_TYPE_H
