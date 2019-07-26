#include "aStar.hpp"
#include <cmath>

namespace ph {

AStar::AStar(const ObstacleGrid& obstacleGrid)
	:mGrid(obstacleGrid)
{
}

Path AStar::getPath(const sf::Vector2u startNodePosition, const sf::Vector2u destinationNodePosition)
{
	std::set<Node> openNodes;
	std::set<Node> closedNodes;

	Node startNode = mGrid.getNodeOfPosition(startNodePosition);
	openNodes.emplace(startNode);

	while(!openNodes.empty()) {
		Node currentNode = *openNodes.begin();
		openNodes.erase(openNodes.begin());
		closedNodes.emplace(currentNode);

		if(currentNode.mPosition == destinationNodePosition)
			return Path();

		for(const Node& neighbour : mGrid.getNeighboursOf(currentNode)) {
			if(neighbour.mIsObstacle || isNodeInSet(neighbour, closedNodes))
				continue;

		}
	}

	return Path(); // temporary
}

bool AStar::isNodeInSet(const Node& node, std::set<Node> set)
{
	auto search = set.find(node);
	return search != set.end();
}

float AStar::getManhatanDistanceToDestination(const sf::Vector2u currentNodePosition)
{
	// Change manhatan distance to some other heuristic when zombie can move in 8 directions
	float legX = std::abs(static_cast<int>(mDestinationNodePosition.x - currentNodePosition.x));
	float legY = std::abs(static_cast<int>(mDestinationNodePosition.y - currentNodePosition.y));
	return legX + legY;
}

}
