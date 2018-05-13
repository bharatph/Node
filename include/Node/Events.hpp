#ifndef _NODE_EVENTS
#define _NODE_EVENTS
namespace node {
enum Events {
	Accept = 0,
	Connect = 1 << 1,
	Reject = 1 << 2,
	Error = 1 << 3,
	Read = 1 << 4,
	ReadLine = 1 << 5
};
}
#endif
