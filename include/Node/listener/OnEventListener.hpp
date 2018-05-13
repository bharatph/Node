#ifndef _NODE_ON_EVENT_LISTENER
#define _NODE_ON_EVENT_LISTENER

#include <Node/Events.hpp>
namespace node {
class OnEventListener {
	public:
	OnEventListener(){
	}
	virtual void onEvent(Events, Node *) = 0;
};
}
#endif
