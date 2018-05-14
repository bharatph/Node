#include <iostream>
#include <Node/Node.hpp>
#include <Node/Events.hpp>
#include <Node/listener/OnEventListener.hpp>

using namespace std;
using namespace node;



class NodeListener : public OnEventListener {
	private:
	void handleNode(Node *n){
		n->setOnEventListener(this);
	}
	public:
	void onEvent(Events events, Node *n) override {
		switch(events){
		case Events::ReadLine | Events::Read:
			cout << n->buffer << endl;
			n->writeln("OK");
			break;
		case Events::Accept:
			handleNode(n);
			break;
		}	
	}
};

int main(int argc, char *argv[]){
	Node *n = new Node();
	n->accept(6555);
	return n->wait(); //TODO add sleep to wait
}
