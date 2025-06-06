#pragma once
#include <rack.hpp>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>

namespace CurveAndDrag {

/**
 * ScalaReader - Class for loading and parsing Scala (.scl) format tuning files
 * 
 * Provides functionality to load microtonal scales from Scala format files and
 * convert between frequencies based on the loaded scale intervals.
 */
class ScalaReader {
public:
    ScalaReader() {
        // Default to 12-TET as fallback
        setDefaultScale();
    }

    /**
     * Load a Scala file (.scl) from the specified path
     * 
     * @param path Path to the Scala file
     * @return true if loaded successfully, false otherwise
     */
    bool loadScalaFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return false;
        }

        // Clear previous scale data
        description.clear();
        cents.clear();

        // Parse Scala file
        std::string line;
        int lineNum = 0;
        int numNotes = 0;

        while (std::getline(file, line)) {
            lineNum++;

            // Skip comments and empty lines
            if (line.empty() || line[0] == '!') {
                continue;
            }

            // First non-comment line is the description
            if (description.empty()) {
                description = line;
                continue;
            }

            // Second non-comment line is the number of notes
            if (numNotes == 0) {
                try {
                    numNotes = std::stoi(line);
                    continue;
                }
                catch (const std::exception& e) {
                    return false;
                }
            }

            // Parse scale degree (cents or ratio)
            if ((int)cents.size() < numNotes) {
                // Remove anything after a comment marker
                size_t commentPos = line.find('!');
                if (commentPos != std::string::npos) {
                    line = line.substr(0, commentPos);
                }

                // Trim whitespace
                line.erase(0, line.find_first_not_of(" \t"));
                line.erase(line.find_last_not_of(" \t") + 1);

                if (line.empty()) {
                    continue;
                }

                // Check if it's a ratio or cents
                if (line.find('/') != std::string::npos) {
                    // Parse as ratio
                    double numerator = 0.0;
                    double denominator = 1.0;
                    sscanf(line.c_str(), "%lf/%lf", &numerator, &denominator);
                    
                    if (denominator != 0.0) {
                        double ratio = numerator / denominator;
                        double centValue = 1200.0 * std::log2(ratio);
                        cents.push_back(centValue);
                    }
                }
                else {
                    // Parse as cents
                    try {
                        double cent = std::stod(line);
                        cents.push_back(cent);
                    }
                    catch (const std::exception& e) {
                        continue;
                    }
                }
            }
        }

        file.close();

        // Add the octave (2/1 ratio = 1200 cents)
        if (!cents.empty()) {
            cents.push_back(1200.0);
            return true;
        }

        // If parsing failed, revert to default scale
        setDefaultScale();
        return false;
    }

    /**
     * Set 12-TET as the default scale
     */
    void setDefaultScale() {
        description = "12-Tone Equal Temperament";
        cents.clear();
        
        // Generate 12-TET scale (12 equal divisions of the octave)
        for (int i = 1; i <= 12; i++) {
            cents.push_back(i * 100.0);
        }
    }

    /**
     * Set 19-EDO as an alternative scale
     */
    void set19EDO() {
        description = "19-Tone Equal Division of the Octave";
        cents.clear();
        
        // Generate 19-EDO scale
        double step = 1200.0 / 19.0;
        for (int i = 1; i <= 19; i++) {
            cents.push_back(i * step);
        }
    }

    /**
     * Get the quantized pitch value based on the loaded scale
     * 
     * @param inputCents Pitch value in cents
     * @return Quantized pitch value in cents according to the scale
     */
    double quantizePitch(double inputCents) {
        // Handle octaves
        double octave = std::floor(inputCents / 1200.0);
        double centsMod = std::fmod(inputCents, 1200.0);
        if (centsMod < 0) {
            centsMod += 1200.0;
            octave -= 1.0;
        }
        
        // Find the closest scale degree
        double closestCent = 0.0;
        double minDistance = 1200.0;
        
        for (double cent : cents) {
            double distance = std::abs(cent - centsMod);
            if (distance < minDistance) {
                minDistance = distance;
                closestCent = cent;
            }
        }
        
        // Return the quantized pitch in cents
        return octave * 1200.0 + closestCent;
    }

    /**
     * Morph between the raw pitch and the quantized pitch
     * 
     * @param rawCents Raw pitch value in cents
     * @param quantizedCents Quantized pitch value in cents
     * @param morphAmount Morph amount (0.0 = raw, 1.0 = quantized)
     * @return Morphed pitch value in cents
     */
    double morphPitch(double rawCents, double quantizedCents, float morphAmount) {
        return rawCents * (1.0 - morphAmount) + quantizedCents * morphAmount;
    }

    /**
     * Get the description of the currently loaded scale
     */
    std::string getDescription() const {
        return description;
    }

private:
    std::string description;
    std::vector<double> cents;
};

} // namespace CurveAndDrag
