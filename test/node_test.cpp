#include <iostream>
#include <Node/Node.hpp>
#include <Node/Events.hpp>
#include <Node/listener/OnEventListener.hpp>

using namespace std;
using namespace node;



class NodeListener : public OnEventListener {
	private:
	void onAccept(Node *n){
		n->setOnEventListener(new NodeListener());
	}
	public:
	void onEvent(Events events, Node *n) override {
		switch(events){
		case Events::ReadLine | Events::Read:
			cout << "CONT:" << n->buffer << endl;
			n->writeln("OK");
			break;
		case Events::Accept:
			onAccept(n);
			break;
		}	
	}
};

int main(int argc, char *argv[]){
	Node *n = new Node();
	n->setOnEventListener(new NodeListener());
	n->setServerPort(5555);
	n->start();
	return n->wait(); //TODO add sleep to wait
}
