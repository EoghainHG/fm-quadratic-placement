#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

struct Pin {
    std::string node_name;
    char pin_dir;
    float x_offset;
    float y_offset;
};

struct Net {
    int degree;
    std::string net_name;
    std::vector<Pin> pins;
};

void parseNetlistFile(const std::string& filename) {
    std::ifstream file(filename);

    // Check if file is opened successfully
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }

    std::string line;
    int numNets = 0, numPins = 0;
    std::vector<Net> nets;

    while (std::getline(file, line)) {
        std::istringstream iss(line);

        // Ignore comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Read number of nets
        if (line.find("NumNets") != std::string::npos) {
            iss.ignore(8); // Skip "NumNets"
            iss >> numNets;
        }
        // Read number of pins
        else if (line.find("NumPins") != std::string::npos) {
            iss.ignore(8); // Skip "NumPins"
            iss >> numPins;
        }
        // Read Net Degree
        else if (line.find("NetDegree") != std::string::npos) {
            Net net;
            iss.ignore(10); // Skip "NetDegree :"
            iss >> net.degree >> net.net_name;

            // Read the next 'degree' lines to capture the pins
            for (int i = 0; i < net.degree; ++i) {
                if (std::getline(file, line)) {
                    std::istringstream pinStream(line);
                    Pin pin;
                    pinStream >> pin.node_name >> pin.pin_dir >> pin.x_offset >> pin.y_offset;
                    net.pins.push_back(pin);
                }
            }
            nets.push_back(net);
        }
    }

    file.close();

    // Output parsed data
    std::cout << "Number of Nets: " << numNets << std::endl;
    std::cout << "Number of Pins: " << numPins << std::endl;
    for (const auto& net : nets) {
        std::cout << "\nNet: " << net.net_name << " (Degree: " << net.degree << ")\n";
        for (const auto& pin : net.pins) {
            std::cout << "  " << pin.node_name << " " << pin.pin_dir 
                      << " (" << pin.x_offset << ", " << pin.y_offset << ")\n";
        }
    }
}
/*
int main() {
    std::string filename = "/Users/oscarbonilla/desktop/Benchmark Files/superblue1/superblue1.nets";
    parseNetlistFile(filename);
    return 0;
}
*/