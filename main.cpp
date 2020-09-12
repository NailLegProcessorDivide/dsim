#include <memory>
#include <vector>
#include <fstream>
#include <time.h>

#define OLC_PGE_APPLICATION

#include "olcPixelGameEngine.h"

const char* NODEHEADER = "id,current pos x,y,start pos x,y,infectable,infected,infected for,max travel,maxSpeed";
const double PI = 3.141592;

double drand() {
	return rand() / (double)RAND_MAX;
}

class Node;

bool infectTest (Node&, Node&);

std::ostream& operator<<(std::ostream& os, const Node& node);

struct WorldParams {
	std::ostream& log;
	int numNodes;
	int minInfTime;
	int maxInfTime;
	double suvRate;
	bool reInfect;
	double minMaxTravel;
	double maxMaxTravel;
	olc::vd2d maxPos;
	olc::vd2d minPos;
	olc::vi2d res;
	olc::vi2d pixSize;
	double maxSpeed;
	int randSeed;
	bool origin;
	bool showRange;
};

class World {
private:
	std::vector<Node> nodes;
public:
	int minInfTime;
	int maxInfTime;
	double suvRate;
	double maxSpeed;
	std::ostream& stream;
	int nextSeed;

	World(WorldParams wParams) : World(wParams.log, wParams.numNodes, wParams.minInfTime, wParams.maxInfTime, wParams.suvRate, wParams.reInfect,
	 wParams.minMaxTravel, wParams.maxMaxTravel, wParams.maxPos, wParams.minPos, wParams.maxSpeed, wParams.randSeed) {}

	World(std::ostream& log, int numNodes, int minInfTime, int maxInfTime, double suvRate, bool reInfect, double minMaxTravel,
	 double maxMaxTravel, olc::vd2d maxPos, olc::vd2d minPos, double maxSpeed, int randSeed = 0) 
	: nodes(), minInfTime(minInfTime), maxInfTime(maxInfTime), suvRate(suvRate), maxSpeed(maxSpeed), stream(log)
	{	
		log << "seed," << randSeed << "\n";
		log << "Node count," << numNodes << "\n";
		log << "min infected time," << minInfTime << "\n";
		log << "max infected time," << maxInfTime << "\n";
		log << "survival rate," << suvRate << "\n";
		log << "reinfect," << reInfect << "\n";
		log << "maxSpeed," << maxSpeed << "\n";

		log << NODEHEADER << "\n";
		srand(randSeed);
		olc::vd2d posDif = maxPos - minPos;
		for (int i = 0; i < numNodes; ++i) {
			olc::vd2d nodePos = minPos + olc::vd2d(drand(), drand()) * posDif;
			double nodeMaxTravel = minMaxTravel + drand() * (maxMaxTravel - minMaxTravel);
			nodes.emplace_back(log, i, nodePos, nodeMaxTravel, maxSpeed, reInfect);
		}
		nextSeed = rand();
	}

	World(World&) = delete;

	void update();

	std::vector<Node>& getNodes() {return nodes;}
};


class Node {
private:
public:
	int id;
	olc::vd2d cPos;
	olc::vd2d sPos;
	bool reinfectable;
	bool infectable;
	bool infected;
	int infectedFor;
	double maxTravel;
	double maxSpeed;

	Node(std::ostream& log, int id, olc::vf2d pos, double maxTravel, float maxSpeed, bool reinfect) :
	 id(id), cPos(pos), sPos(pos), reinfectable(reinfect), infectable(true), infected(false),
	 infectedFor(0), maxTravel(maxTravel), maxSpeed(maxSpeed) {
		 log << *this;
	}

	void move() {
		double dir = drand() * PI * 2;
		double dist = drand() * maxSpeed;
		olc::vd2d posOff = olc::vd2d(sin(dir), cos(dir)) * dist;
		double ndist = (posOff + cPos - sPos).mag();
		cPos = sPos;
		if (ndist != 0) {
			cPos += (posOff + cPos - sPos).norm()*std::min(ndist, maxTravel);
		}
	}

	void update(World& world) {
		for (Node& node : world.getNodes()) {
			if (node.id != id && !infected) {
				infected = infectTest(*this, node);
			}
		}
	}

	friend std::ostream& operator<<(std::ostream& os, const Node& dt);
};

void World::update(){
	srand(nextSeed);
	stream << NODEHEADER << "\n";
	for (auto& node : nodes) {
		node.move();
	}
	for (auto& node : nodes) {
		node.update(*this);
		stream << node;
	}
	nextSeed = rand();
}

bool infectTest (Node& n1, Node& n2) {

	return (n1.cPos-n2.cPos).mag() < 4 ? n2.infected : false;
}

std::ostream& operator<<(std::ostream& os, const Node& node)
{
    os << node.id << "," << node.cPos << "," << node.sPos << "," << node.infectable << "," << node.infected << "," <<
	 node.infectedFor << "," << node.maxTravel << "," << node.maxSpeed << "\n";
    return os;
}

class Stats {
public:
	void collect(World& world);
};

class DSim : public olc::PixelGameEngine {
private:
	double scale;

public:
	World world;
	bool running;
	WorldParams& wParams;
	DSim(WorldParams& wParams) : world(wParams), wParams(wParams),
				running(false)
	{
		sAppName = "dSim";
		scale = 32;
		auto& nodes = world.getNodes();
		nodes[0].infected = true;
	}

public:
	bool OnUserCreate() override
	{

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		if (GetKey(olc::SPACE).bPressed) {
			running = !running;
		}
		if (running || GetKey(olc::R).bPressed) {
			world.update();

		}
		Clear(olc::BLACK);
		for (Node& node : world.getNodes()) {
			if (wParams.origin) DrawCircle(node.sPos, 2, olc::GREEN);
			DrawCircle(node.cPos, 2, node.infected ? olc::RED : olc::WHITE);
			if (wParams.showRange) DrawCircle(node.sPos, node.maxTravel, olc::VERY_DARK_GREY);
		}
		return true;
	}
};

int main(int argc, char** argv)
{
	WorldParams wp{std::cout, 3000, 5, 20, 0.8, false, 3, 10, olc::vd2d(500, 500), olc::vd2d(0,0), olc::vi2d(512, 512), olc::vi2d(2, 2), 1, time(0), false, false};
	for (int i = 1; i < argc; ++i) {
		int falls = 0;
		if (argc - i <= 1) {//no args
			if (strcmp(argv[i], "--reInf") == 0) {
				wp.reInfect = true;
			}
			else if (strcmp(argv[i], "--showRange") == 0) {
				wp.showRange = true;
			}
			else if (strcmp(argv[i], "--showOrigins") == 0) {
				wp.origin = true;
			}
			else {
				++falls;
			}
		}
		if (argc - i >= 2) { //1 arg
			if (strcmp(argv[i], "--file") == 0) {
				++i;
			}
			else if (strcmp(argv[i], "--nodeCount") == 0) {
				wp.numNodes = std::stoi(argv[++i]);
			}
			else if (strcmp(argv[i], "--minMaxTravel") == 0) {
				wp.minMaxTravel = std::stoi(argv[++i]);
			}
			else if (strcmp(argv[i], "--maxMaxTravel") == 0) {
				wp.maxMaxTravel = std::stoi(argv[++i]);
			}
			else if (strcmp(argv[i], "--suvRate") == 0) {
				wp.suvRate = std::stod(argv[++i]);
			}
			else if (strcmp(argv[i], "--minMaxTravel") == 0) {
				wp.minMaxTravel = std::stod(argv[++i]);
			}
			else if (strcmp(argv[i], "--maxMaxTravel") == 0) {
				wp.maxMaxTravel = std::stod(argv[++i]);
			}
			else if (strcmp(argv[i], "--minPosX") == 0) {
				wp.minPos.x = std::stod(argv[++i]);
			}
			else if (strcmp(argv[i], "--minPosY") == 0) {
				wp.minPos.y = std::stod(argv[++i]);
			}
			else if (strcmp(argv[i], "--maxPosX") == 0) {
				wp.maxPos.x = std::stod(argv[++i]);
			}
			else if (strcmp(argv[i], "--maxPosY") == 0) {
				wp.maxPos.y = std::stod(argv[++i]);
			}
			else if (strcmp(argv[i], "--resX") == 0) {
				wp.res.x = std::stoi(argv[++i]);
			}
			else if (strcmp(argv[i], "--resY") == 0) {
				wp.res.y = std::stoi(argv[++i]);
			}
			else if (strcmp(argv[i], "--pixX") == 0) {
				wp.pixSize.x = std::stoi(argv[++i]);
			}
			else if (strcmp(argv[i], "--pixY") == 0) {
				wp.pixSize.y = std::stoi(argv[++i]);
			}
			else if(strcmp(argv[i], "--maxSpeed") == 0) {
				wp.maxSpeed = std::stod(argv[++i]);
			}
			else if(strcmp(argv[i], "--seed") == 0) {
				wp.randSeed = std::stoi(argv[++i]);
			}
			else {
				++falls;
			}
		}
		if (falls == 2) {
			std::cout << "unknown argument: " << argv[i] << std::endl;
			return 1;
		}
	}

	DSim demo{wp};
	if (demo.Construct(wp.res.x, wp.res.y, wp.pixSize.x, wp.pixSize.y, false, false))
		demo.Start();

	return 0;
}