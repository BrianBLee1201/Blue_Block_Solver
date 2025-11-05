#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <queue>
#include <unordered_map>
#include <string>
#include <sstream>
#include <ApplicationServices/ApplicationServices.h>
#include <iomanip>
#include <unistd.h>
#include <CoreGraphics/CoreGraphics.h>

const int N = 6;

enum class Direction { LEFT, RIGHT, UP, DOWN };

struct Block {
    int row, col;
    int length;
    bool isHorizontal;
    bool isBlue;

    bool operator==(const Block &other) const {
        return row == other.row && col == other.col &&
               length == other.length && isHorizontal == other.isHorizontal && isBlue == other.isBlue;
    }
};

using BoardState = std::vector<Block>;

struct State {
    BoardState blocks;
    std::vector<std::string> path;
    bool blueBlockRemoved = false;

    std::string hash() const {
        std::ostringstream oss;
        for (const auto &b : blocks) {
            oss << b.row << "," << b.col << "," << b.length << "," << b.isHorizontal << "," << b.isBlue << ";";
        }
        return oss.str();
    }
};

bool isTargetReached(const Block &b) {
    return b.isBlue && (b.col + b.length - 1 >= N);
}

bool checkCollision(const BoardState &blocks, int idx, Direction dir, int step) {
    const Block &b = blocks[idx];
    int dr = 0, dc = 0;
    switch (dir) {
        case Direction::LEFT:  dc = -1; break;
        case Direction::RIGHT: dc = 1; break;
        case Direction::UP:    dr = -1; break;
        case Direction::DOWN:  dr = 1; break;
    }

    // Copy the current block state
    int r = b.row;
    int c = b.col;

    for (int s = 1; s <= step; ++s) {
        r += dr;
        c += dc;

        // Check bounds for all parts of the block
        for (int k = 0; k < b.length; ++k) {
            int nr = r + (b.isHorizontal ? 0 : k);
            int nc = c + (b.isHorizontal ? k : 0);

            if (nr < 0 || nr >= N || nc < 0 || nc >= N)
                return false;

            // Check for collision with other blocks
            for (int i = 0; i < blocks.size(); ++i) {
                if (i == idx) continue;
                const Block &o = blocks[i];
                for (int ok = 0; ok < o.length; ++ok) {
                    int orow = o.row + (o.isHorizontal ? 0 : ok);
                    int ocol = o.col + (o.isHorizontal ? ok : 0);
                    if (orow == nr && ocol == nc)
                        return false;
                }
            }
        }
    }

    return true;
}

BoardState moveBlock(const BoardState &blocks, int idx, Direction dir) {
    BoardState newBlocks = blocks;
    Block &b = newBlocks[idx];
    switch (dir) {
        case Direction::LEFT:  b.col--; break;
        case Direction::RIGHT: b.col++; break;
        case Direction::UP:    b.row--; break;
        case Direction::DOWN:  b.row++; break;
    }
    return newBlocks;
}

BoardState moveBlock(const BoardState &blocks, int idx, Direction dir, int step) {
    BoardState newBlocks = blocks;
    Block &b = newBlocks[idx];
    switch (dir) {
        case Direction::LEFT:  b.col -= step; break;
        case Direction::RIGHT: b.col += step; break;
        case Direction::UP:    b.row -= step; break;
        case Direction::DOWN:  b.row += step; break;
    }
    return newBlocks;
}

bool isSolved(const BoardState &blocks) {
    for (const Block &b : blocks) {
        if (b.isBlue)
            return false;
    }
    return true;
}

void printBoard(const BoardState &blocks) {
    std::string grid[N][N] = {};
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            grid[i][j] = ".";

    for (int i = 0; i < blocks.size(); ++i) {
        const auto &b = blocks[i];
        std::string ch;
        ch = std::to_string(i);
        for (int k = 0; k < b.length; ++k) {
            int r = b.row + (b.isHorizontal ? 0 : k);
            int c = b.col + (b.isHorizontal ? k : 0);
            if (r >= 0 && r < N && c >= 0 && c < N)
                grid[r][c] = ch;
        }
    }

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j)
            std::cout << std::setw(2) << std::right << grid[i][j] << ' ';
        std::cout << '\n';
    }
    std::cout << '\n';
}


void pushMove(const State &cur, int idx, Direction dir,
    std::queue<State> &q, std::unordered_map<std::string, int> &visited) {
    BoardState next = moveBlock(cur.blocks, idx, dir);
    std::ostringstream move;
    move << "B" << idx
         << (dir == Direction::LEFT ? "L" : dir == Direction::RIGHT ? "R" :
             dir == Direction::UP ? "U" : "D") << 1;
    std::vector<std::string> newPath = cur.path;
    newPath.push_back(move.str());
    State nextState{next, newPath};
    std::string hashVal = nextState.hash();
    int newCost = static_cast<int>(newPath.size());
    if (!visited.count(hashVal) || newCost < visited[hashVal]) {
        visited[hashVal] = newCost;
        q.push(nextState);
    }
}

std::vector<std::string> bfs(const BoardState &initial) {
    std::queue<State> q;
    std::unordered_map<std::string, int> visited;

    State start{initial, {}};
    q.push(start);
    visited[start.hash()] = 0;

    while (!q.empty()) {
        State current = q.front(); q.pop();
        if (isSolved(current.blocks)) return current.path;

        for (int i = 0; i < current.blocks.size(); ++i) {
            Block &b = current.blocks[i];
            std::vector<Direction> directions = b.isHorizontal
                ? std::vector<Direction>{Direction::LEFT, Direction::RIGHT}
                : std::vector<Direction>{Direction::UP, Direction::DOWN};

            for (Direction dir : directions) {
                for (int step = 1; step < N; ++step) {
                    if (!checkCollision(current.blocks, i, dir, step)) break;
                    BoardState moved = moveBlock(current.blocks, i, dir, step);

                    std::vector<std::string> newPath = current.path;
                    bool newRemoved = current.blueBlockRemoved;

                    auto oldSize = moved.size();
                    moved.erase(std::remove_if(moved.begin(), moved.end(), [](const Block& b) {
                        return b.isBlue && b.row == 2 && b.col == 4 && b.length == 2 && b.isHorizontal;
                    }), moved.end());
                    if (moved.size() < oldSize) {
                        newRemoved = true;
                        //newPath[newPath.size()-1][newPath[newPath.size()-1].size() - 1] += 1;
                    }

                    std::ostringstream move;
                    if (current.blueBlockRemoved == true){
                        if (current.blocks[i].isBlue){
                            move << "B0";
                        }
                        else{
                            move << "B" << std::to_string(i+1);
                        }
                    }
                    else{
                        move << "B" << std::to_string(i);
                    }
                    move << (dir == Direction::LEFT ? "L" : dir == Direction::RIGHT ? "R" :
                        dir == Direction::UP ? "U" : "D") << step;

                    newPath.push_back(move.str());
                    State nextState{moved, newPath, newRemoved};
                    std::string hashVal = nextState.hash();
                    int newCost = static_cast<int>(newPath.size());
                    if (!visited.count(hashVal) || newCost < visited[hashVal]) {
                        visited[hashVal] = newCost;
                        q.push(nextState);
                    }
                }
            }
        }
    }
    return {"No solution."};
}

// Helper function to compress the path by merging consecutive moves of the same block in the same direction.
std::string compressPath(const std::vector<std::string> &rawPath) {
    std::string lastPrefix = "";
    int totalSteps = 0;
    std::ostringstream compressed;

    for (const auto& token : rawPath) {
        std::string prefix = token.substr(0, token.length() - 1);
        int step = token.back() - '0';

        if (prefix == lastPrefix) {
            totalSteps += step;
        } else {
            if (!lastPrefix.empty()) {
                compressed << lastPrefix << totalSteps << " ";
            }
            lastPrefix = prefix;
            totalSteps = step;
        }
    }

    if (!lastPrefix.empty()) {
        compressed << lastPrefix << totalSteps << " ";
    }

    return compressed.str();
}

void performMouseAutomation(const std::vector<std::string>& solution, const BoardState& blocks) {
    // Bring Blue Block game window to the front and wait for focus
    system("osascript -e 'tell application \"System Events\" to tell process \"Blue Block\" to set frontmost to true'");
    //usleep(100000); // Wait for the window to focus
    const int squareSizeX = (1462 - 1127) / 5;
    const int squareSizeY = (726 - 390) / 5;
    auto getCenter = [&](int row, int col) {
        int x = 1127 + col * squareSizeX;
        int y = 390 + row * squareSizeY;
        return CGPointMake(x, y);
    };

    BoardState currentBlocks = blocks;
    for (const auto& move : solution) {
        if (move.length() < 4 || move[0] != 'B') continue;

        int idx = move[1] - '0';
        if (isdigit(move[2])) {
            idx = std::stoi(move.substr(1, move.find_first_not_of("0123456789", 1)));
        }

        char dir = move[move.size() - 2];
        int step = move.back() - '0';

        if (idx >= currentBlocks.size()) continue;
        Block& b = currentBlocks[idx];

        int r = b.row + (b.isHorizontal ? 0 : 0);
        int c = b.col + (b.isHorizontal ? 0 : 0);
        CGPoint start = getCenter(r, c);

        int dr = 0, dc = 0;
        switch (dir) {
            case 'L': dc = -step; break;
            case 'R': dc = step; break;
            case 'U': dr = -step; break;
            case 'D': dr = step; break;
        }

        CGPoint end = getCenter(r + dr, c + dc);

        CGEventRef move1 = CGEventCreateMouseEvent(NULL, kCGEventMouseMoved, start, kCGMouseButtonLeft);
        CGEventRef click1 = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, start, kCGMouseButtonLeft);
        CGEventRef drag = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDragged, end, kCGMouseButtonLeft);
        CGEventRef release = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp, end, kCGMouseButtonLeft);

        CGEventPost(kCGHIDEventTap, move1);
        usleep(5000); // after move
        CGEventPost(kCGHIDEventTap, click1);
        usleep(5000); // after click
        CGEventPost(kCGHIDEventTap, drag);
        usleep(10000); // after drag
        CGEventPost(kCGHIDEventTap, release);
        //CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.01, false);

        CFRelease(move1);
        CFRelease(click1);
        CFRelease(drag);
        CFRelease(release);

        // Update block state
        switch (dir) {
            case 'L': b.col -= step; break;
            case 'R': b.col += step; break;
            case 'U': b.row -= step; break;
            case 'D': b.row += step; break;
        }

        //usleep(10000); // Wait after each move
    }
}

cv::Mat captureScreenRegion() {
    const char* filename = "/tmp/blueblock_capture.png";

    // Capture a region (x=1095, y=360, w=398, h=398)
    std::string command = "screencapture -R1095,360,398,398 -x " + std::string(filename);
    int result = system(command.c_str());
    if (result != 0) {
        std::cerr << "Failed to capture screen using screencapture.\n";
        return {};
    }

    cv::Mat img = cv::imread(filename);
    if (img.empty()) {
        std::cerr << "Failed to load captured screenshot.\n";
    }
    return img;
}

// Detect blocks from an input screenshot using OpenCV.
BoardState detectBlocksFromImage(const cv::Mat &img) {
    const cv::Point topLeft(1095, 360);
    const cv::Point bottomRight(1493, 758);
    BoardState blocks;
    // Template matching for blue dragon blocks
    std::vector<std::string> templates = {
        "dragon_template_A.png", "dragon_template_B.png"
    };

    for (const std::string& templFile : templates) {
        cv::Mat templ = cv::imread(templFile);
        if (templ.empty()) {
            std::cerr << "Failed to load template: " << templFile << std::endl;
            continue;
        }

        cv::Mat result;
        cv::matchTemplate(img, templ, result, cv::TM_CCOEFF_NORMED);
        double threshold = 0.8;
        bool inserted = false;

        for (int y = 0; y < result.rows; ++y) {
            if (inserted){
                break;
            }
            for (int x = 0; x < result.cols; ++x) {
                if (result.at<float>(y, x) >= threshold) {
                    int row = y / 140;
                    int col = x / 140;
                    if (row >= 0 && row < N && col >= 0 && col + 1 < N) {
                        blocks.push_back(Block{row, col, 2, true, true});
                        cv::rectangle(img, cv::Point(x, y),
                                      cv::Point(x + templ.cols, y + templ.rows),
                                      cv::Scalar(255, 0, 255), 2);
                        inserted = true;
                        break;
                    }
                }
            }
        }
    }

    // HSV-based detection for obstacle blocks (updated RGB approx. 22–64, 21–53, 15–41)
    cv::Mat hsv;
    cv::cvtColor(img, hsv, cv::COLOR_BGR2HSV);

    cv::Scalar obstacle_lower(5, 50, 20);
    cv::Scalar obstacle_upper(30, 140, 100);
    cv::Mat beige_mask;
    cv::inRange(hsv, obstacle_lower, obstacle_upper, beige_mask);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(beige_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (const auto& cnt : contours) {
        cv::Rect rect = cv::boundingRect(cnt);
        int x = rect.x, y = rect.y, w = rect.width, h = rect.height;
        cv::rectangle(img, rect, cv::Scalar(0, 0, 255), 2); // Red box for debugging

        int length_pixels = std::max(w, h);
        int thickness_pixels = std::min(w, h);
        // if (length_pixels >= 110 && length_pixels <= 400 && thickness_pixels <= 130 && thickness_pixels >= 110) {
        //     std::cout << "Detected specific contour at (x=" << (1095 + x/2) << ", y=" << (y/2 + 360)
        //     << ", w=" << (w/2) << ", h=" << (h/2) << ")\n";
        // }

        if (length_pixels >= 100 && length_pixels <= 400 && thickness_pixels <= 140 && thickness_pixels >= 100) {
            bool isHorizontal = w > h;
            int length = (length_pixels < 300) ? 2 : 3;
            int row = y / 130;
            int col = x / 130;
            int projected_row = row;
            int projected_col = col;
            if (isHorizontal){
                projected_col += (length - 1);
            }
            else{
                projected_row += (length - 1);
            }

            if (row >= 0 && row < N && col >= 0 && col < N && projected_col <= 5 && projected_row <= 5 && (length == 2 || length == 3)) {
                Block b{row, col, length, isHorizontal, false};

                // Prevent duplicate blocks
                bool exists = false;
                for (const auto& other : blocks) {
                    if (other.row == b.row && other.col == b.col && other.length == b.length &&
                        other.isHorizontal == b.isHorizontal && other.isBlue == b.isBlue) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    blocks.push_back(b);
                    // std::cout << "Length of block in pixels is " << length_pixels << "\n";
                    // std::cout << "This puts at row " << row << " column " << col << " with length " << length << "\n";
                    // std::cout << "When drawing rectangle it was " << w << " and " << h << "\n";
                    cv::rectangle(img, rect, cv::Scalar(0, 255, 0), 2);
                }
            }
        }
    }
    cv::imwrite("debug_contours.png", img);

    return blocks;
}
// Example usage
const int NUM_GAMES = 3674;
const int COOLDOWN = 1100000; //1200000 for single block
int main() {
    for (int a = 0; a < NUM_GAMES; a++){
        cv::Mat img = captureScreenRegion();
        if (img.empty()) {
            std::cerr << "Failed to capture image.\n";
            return 1;
        }
        cv::imwrite("debug_capture.png", img);
        std::cout << "Saved debug image to debug_capture.png\n";
    
        BoardState board = detectBlocksFromImage(img);
        printBoard(board);
    
        std::vector<std::string> solution = bfs(board);
        if (!solution.empty()){
            std::ostringstream oss;
            for (const auto& move : solution) oss << move << " ";
            std::cout << "Solution: " << oss.str() << std::endl;
            performMouseAutomation(solution, board);
        }
        else{
            break;
        }
        usleep(COOLDOWN);
    }
    return 0;
}