#include "HolonomicHeuristic.hpp"

#include <limits>

using namespace astar;

// basic constructor
HolonomicHeuristic::HolonomicHeuristic(InternalGridMapRef map) :
    grid(map),
    start(),
    goal(),
    twopi(2.0*M_PI),
    circle_path(),
    nearest_open(),
    largest_open(),
    closed()
{
    piece_angle = twopi / 36.0;
}

// verify if two circles overlaps with each other
bool HolonomicHeuristic::Overlap(const astar::Circle &a, const astar::Circle &b, double factor) {

    double smaller, greater;

    if (a.r > b.r) {
        smaller = b.r;
        greater = a.r;
    } else {
        smaller = a.r;
        greater = b.r;
    }

    return ((a.position.Distance(b.position) - greater) < (factor * smaller));

}

// get the nearest circle from a given pose
HolonomicHeuristic::CircleNodePtr HolonomicHeuristic::NearestCircleNode(const astar::Pose2D &p) {

    // stupid search
    if (0 < circle_path.circles.size()) {

        // get a direct access
        std::vector<HolonomicHeuristic::CircleNodePtr> &circles(circle_path.circles);

        // the Vector2D access
        Vector2D<double> position(p.position);

        // iterators
        std::vector<HolonomicHeuristic::CircleNodePtr>::iterator it = circles.begin();

        // set the first circle as the closer one
        std::vector<HolonomicHeuristic::CircleNodePtr>::iterator nearest = it;

        // get the end iterator
        std::vector<HolonomicHeuristic::CircleNodePtr>::iterator end = circles.end();
        std::vector<HolonomicHeuristic::CircleNodePtr>::iterator next;

        // get the current distance between the pose and the first circle
        double distance = position.Distance((*it)->circle.position);

        // update the current iterator
        ++it;

        // iterate along the circle path
        while (it != end) {

            double d = position.Distance((*it)->circle.position);

            if (d < distance) {

                // update the values
                distance = d;
                nearest = it;

            }

            // update the iterator
            ++it;

        }

        // we got it!
        next = nearest;
        ++next;

        if (next != end){

            // get the position vectors reference
            astar::Vector2D<double> &next_circle((*next)->circle.position);
            astar::Vector2D<double> &current_circle((*nearest)->circle.position);

            if (current_circle.Distance2(next_circle) > position.Distance2(next_circle)) {

                return *next;

            }

        }

        return *nearest;

    }

    return nullptr;

}

// get the circle children
HolonomicHeuristic::CircleNodePtrArrayPtr HolonomicHeuristic::GetChildren(HolonomicHeuristic::CircleNodePtr cn) {

    // the resulting circles
    HolonomicHeuristic::CircleNodePtrArrayPtr children = new HolonomicHeuristic::CircleNodePtrArray();

    // get the direct access
    std::vector<HolonomicHeuristic::CircleNodePtr> &circles(children->circles);

    // the current parent circle positionc coordinates
    double x = cn->circle.position.x;
    double y = cn->circle.position.y;

    // the current parent circle radius
    double r = cn->circle.r;

    // the next child position
    Vector2D<double> ncp;

    // the next child radius
    double ncr;

    // the next child distance from the parent node
    double ncg;

    // the next child distance to the goal
    double ncf;

    // the start angle
    double angle = 0.0;

    // rotate around the current circle and get the valid circles
    while (angle <= twopi) {

        // get the current position
        ncp.x = x + r * std::cos(angle);
        ncp.y = y + r * std::sin(angle);

        if (grid.isValidPoint(ncp)) {

            // get the child radius
            ncr = grid.GetObstacleDistance(ncp) - 0.25;

            // is it a safe place?
            if (ncr > 1.5) {

                // get the distance from the parent node
                ncg = r + cn->g;

                // get the distance to the goal
                ncf = ncp.Distance(goal.position) + ncg;

                // save the circle to the children list
                circles.push_back(new CircleNode(Circle(ncp, ncr), ncr, ncg, ncf, cn));

            }

        }

        // update the angle
        angle += piece_angle;

    }

    return children;
}

// clear the current CircleNodePtr sets
void HolonomicHeuristic::RemoveAllCircleNodes() {

    // a helper pointer
    HolonomicHeuristic::CircleNodePtr tmp;

    // clear the current open set
    while (!nearest_open.empty()) {

        // remove the min from the priority queue
        tmp = nearest_open.top();

        // remove the element from the heap
        nearest_open.pop();

        if (!tmp->explored) {

            // delete from memmory
            delete tmp;

        }

    }

    // clear the largest set
    while(!largest_open.empty()) {

        largest_open.pop();

    }

    while(!closed.empty()) {

        // get the last node
        tmp = closed.back();

        // remove from the vector
        closed.pop_back();

        // remove from the heap
        delete tmp;

    }

}

// rebuild the circle array
void HolonomicHeuristic::RebuildCirclePath(HolonomicHeuristic::CircleNodePtr cn) {

    // get the direct access
    std::vector<HolonomicHeuristic::CircleNodePtr> &circles(circle_path.circles);

    // a temp circle node
    HolonomicHeuristic::CircleNodePtr tmp = nullptr;

    // the circle node list, just to prepend the actual circle nodes
    std::list<CircleNodePtr> l;

    // build the goal CircleNode
    tmp = new CircleNode(*cn);

    // append to the list
    l.push_front(tmp);

    while (nullptr != cn) {

        // update the distance between the current node and the it's child
        cn->g = tmp->circle.position.Distance(cn->circle.position) + tmp->g;

        // prepend to the circle path list
        l.push_front(cn);

        // go to the parent node
        tmp = cn;
        cn = cn->parent;

    }

    // copy the entire list to the vector
    if (0 < l.size()) {

        // ge the first iterator
        std::list<CircleNodePtr>::iterator it = l.begin();
        std::list<CircleNodePtr>::iterator end = l.end();

        // get the current circle
        HolonomicHeuristic::CircleNodePtr parent_c_node = nullptr;
        HolonomicHeuristic::CircleNodePtr child_c_node = nullptr;

        while (it != end) {

            // build the next circle
            child_c_node = new HolonomicHeuristic::CircleNode(*(*it));

            // update the parent node
            child_c_node->parent = parent_c_node;

            // append to the circle path
            circles.push_back(child_c_node);

            parent_c_node = child_c_node;

            // update the current iterator
            ++it;

        }

        // delete the last element in the list
        // the last circle was allocated inside this method
        // so it'll not be removed by the RemoveAllCircleNodes() method
        tmp = l.back();
        l.pop_back();
        delete tmp;

    }

}

// try to find a circle node inside the closed set
bool HolonomicHeuristic::NotExist(HolonomicHeuristic::CircleNodePtr cn) {

    // stupid search
    if (0 < closed.size()) {

        // the Vector2D access
        Vector2D<double> &position(cn->circle.position);

        // iterators
        std::vector<CircleNodePtr>::iterator it = closed.begin();

        // get the end iterator
        std::vector<CircleNodePtr>::iterator end = closed.end();

        // get the direct access
        Circle &c(cn->circle);

        double cng = cn->g;

        // iterate along the circle path
        while (it != end) {

            // dereference
            HolonomicHeuristic::CircleNodePtr tmp = *it;

            // verify if the circles are related
            if (tmp != cn->parent && Overlap(c, tmp->circle, 0.1)) {

                return false;

            }


            // update the iterator
            ++it;

        }

    }

    return true;

}

// explore a given circle node
void HolonomicHeuristic::ExploreCircleNode(CircleNodePtr cn) {

    // get the children nodes
    HolonomicHeuristic::CircleNodePtrArrayPtr children = GetChildren(cn);

    if (0 < children->circles.size()) {

        // get the direct access
        std::vector<HolonomicHeuristic::CircleNodePtr> &circles(children->circles);

        // get the vector size
        unsigned int c_size = circles.size();

        // build the iterators
        std::vector<HolonomicHeuristic::CircleNodePtr>::iterator it = circles.begin();
        std::vector<HolonomicHeuristic::CircleNodePtr>::iterator end = circles.end();

        while (it != end) {

            // get the child pointer
            HolonomicHeuristic::CircleNodePtr child = *it;

            if (NotExist(child)) {

                // add the child to the nearest priority queue
                nearest_open.push(child);

                // add the child to the largest priority queue
                largest_open.push(child);

            } else {

                // discard the current child
                delete child;

            }

            // update the iterator
            ++it;
        }

    }

    delete children;

}

// process a given node
bool HolonomicHeuristic::ProcessNode(CircleNodeRef goal, CircleNodePtr cn) {

    // append to the closed set
    closed.push_back(cn);

    // mark as explored, so the we can avoid duplicated explorations
    cn->explored = true;

    // get the adjacent circles
    ExploreCircleNode(cn);

    // verify the overlap case
    if (Overlap(cn->circle, goal.circle, 0.5)) {

        goal.parent = cn;

        // success!
        RebuildCirclePath(&goal);

        // Show Circle Path
        ShowCirclePath();

        // clear the open and closed sets
        RemoveAllCircleNodes();

        return true;

    }

    return false;

}

// show the entire circle path t
void HolonomicHeuristic::ShowCirclePath() {

    //
    cv::namedWindow("Circles", cv::WINDOW_AUTOSIZE);

    unsigned int w = grid.GetWidth();
    unsigned int h = grid.GetHeight();

    unsigned char *map = grid.GetGridMap();

    cv::Mat map_image(w, h, CV_8UC1, map);

    // draw the circles
    for (unsigned int i = 0; i < circle_path.circles.size(); ++i) {

        astar::GridCellIndex index(grid.PoseToIndex(circle_path.circles[i]->circle.position));

        // conver to cv point
        cv::Point p(index.col, h - index.row);

        cv::circle(map_image, p, circle_path.circles[i]->circle.r * 5, cv::Scalar(0.0, 0.0, 0.0), 1);
        cv::imshow("Circles", map_image);
        cv::waitKey(100);

    }

    // destroy the window
    cv::destroyWindow("Circles");

    delete [] map;

}

// the current search method
bool HolonomicHeuristic::SpaceExploration() {

    // set the start circle
    Circle c_start(start.position, grid.GetObstacleDistance(start.position));

    // build the goal circle
    Circle c_goal(goal.position, grid.GetObstacleDistance(goal.position));

    // the heuristic values
    double f = c_start.position.Distance(goal.position);

    // build a new CircleNode
    HolonomicHeuristic::CircleNodePtr cn = new HolonomicHeuristic::CircleNode(c_start, c_start.r, 0.0, f, nullptr);

    // build the goal CircleNode
    HolonomicHeuristic::CircleNode gcn(c_goal, c_goal.r, 0.0, 0.0, nullptr);

    // add to the nearest open priority queue
    nearest_open.push(cn);

    // add to the largest open priority queue
    largest_open.push(cn);

    // reset the status

    // the main loop
    while (!nearest_open.empty()) {

        // pop the min element
        //cn = open.top();
        cn = nearest_open.top();

        // remove the top element from the queue
        nearest_open.pop();

        if (!cn->explored) {

            // process the current circle node
            // if it's a valid path to the goal it will return true
            // otherwise we got a false value
            if (ProcessNode(gcn, cn)) {

                return true;

            }

        }

        // explore the largest node set
        // get the largest node
        cn = largest_open.top();

        // remove from the queue
        largest_open.pop();

        if (!cn->explored) {

            // process the current circle node
            // if it's a valid path to the goal it will return true
            // otherwise we got a false value
            if (ProcessNode(gcn, cn)) {

                return true;

            }

        }

    }

    // remove the nodes
    RemoveAllCircleNodes();

    // there is not a free path to the goal
    return false;

}

// THE HEURISTIC OBJECT METHODS

// update the Heuristic circle path with a new grid map, start and goal poses
void HolonomicHeuristic::UpdateHeuristic(
        astar::InternalGridMapRef grid_map, const astar::Pose2D &start_, const astar::Pose2D &goal_) {

    if (grid_map.HasChanged() || goal != goal_) {

        // update the grid reference
        grid = grid_map;

        // copy the new start pose
        start = start_;

        // save the next goal pose
        goal = goal_;

        // clear the old circle path
        circle_path.circles.clear();

        // build the new circle path
        if (!SpaceExploration()) {

            std::cout << "Could no find a circle path connecting the start and goal poses!\n";

        }

    }

}

// get the heuristic value given a pose
double HolonomicHeuristic::GetHeuristicValue(const astar::Pose2D &p) {

    // get the nearest circle
    HolonomicHeuristic::CircleNodePtr nearest = NearestCircleNode(p);

    if (nullptr != nearest) {
        return nearest->g + p.position.Distance(nearest->circle.position);
    }

    // return the old euclidean distance value
    return goal.position.Distance(p.position);

}
