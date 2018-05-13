#include <iostream>
#include <Node/Node.hpp>
#include <Node/Events.hpp>
#include <Node/listener/OnEventListener.hpp>

using namespace std;
using namespace node;

class NodeListener : public OnEventListener {
	public:
	void onEvent(Events events, Node *n) override {
		if(events == Events::ReadLine){
			cout << "Started Reading" << endl;
			n->writeln("OK");
		}
	}	
};

int main(int argc, char *argv[]){
	Node *n = new Node();
	Node *cli = n->accept(6555);
	cli->setOnEventListener(Events::ReadLine, new NodeListener()); 
	cli->readln();
	return 0;
}
