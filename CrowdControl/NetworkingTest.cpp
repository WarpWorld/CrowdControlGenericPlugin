#include "CrowdControl.hpp"

// Sends a WebSocket message and prints the response
int main(int argc, char** argv) {
	CrowdControl* cc = new CrowdControl();
	int result = cc->Run();
	delete cc; 
	return result;
}
