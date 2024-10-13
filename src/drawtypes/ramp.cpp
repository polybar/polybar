#include <vector>
#include <string>
#include <memory>
#include <iostream>

class ColorRamp {
public:
    using color_t = std::string; // Define color as a string for simplicity

    // Add a color to the ramp
    void add(color_t&& color) {
        m_colors.emplace_back(std::forward<color_t>(color));
    }

    // Get a color by index
    color_t get(size_t index) {
        return m_colors[index];
    }

    // Get a color based on a percentage
    color_t get_by_percentage(float percentage) {
        size_t index = static_cast<size_t>(percentage * m_colors.size() / 100.0f);
        return m_colors[cap(index, 0, m_colors.size() - 1)];
    }

    // Check if the ramp is empty
    operator bool() {
        return !m_colors.empty();
    }

private:
    std::vector<color_t> m_colors; // Vector to store colors

    // Helper function to cap the index
    size_t cap(size_t value, size_t min, size_t max) {
        return (value < min) ? min : (value > max) ? max : value;
    }
};

// Example usage
int main() {
    ColorRamp ramp;
    ramp.add("Red");
    ramp.add("Green");
    ramp.add("Blue");

    std::cout << "Color at 50%: " << ramp.get_by_percentage(50) << std::endl; // Should output "Green"
    std::cout << "Color at 100%: " << ramp.get_by_percentage(100) << std::endl; // Should output "Blue"

    return 0;
}
