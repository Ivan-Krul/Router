#include <iostream>
#include <vector>
#include <algorithm>
#include <array>
#include <fstream>
#include <string>

#include <math.h>

typedef unsigned int INDEX;

struct Node;
struct Wire;

struct Client
{
	INDEX index = 0;
	std::string resource;
	Wire* wire;
};
struct Node
{
	INDEX index = 0;
	unsigned int max_wires = 0;
	std::vector<Wire> wires;
	std::vector<Client*> related_clients;
};
struct Wire
{
	unsigned int length;
	unsigned int capacity;
	Client* client;
	Node* node;
	Node* to_node;
};
struct Query
{
	INDEX index_from;
	INDEX index_to;
	std::string message;
};
struct Network
{
	std::vector<Client> clients;
	std::vector<Node> nodes;
};

void GenerateRelatedNodes(Node* node)
{
	for(unsigned int i = 0; i < node->wires.size(); i++)
	{
		if(node->wires[i].client)
			node->related_clients.push_back(node->wires[i].client);
	}
}

int CalculateCost(Node* node)
{
	int cost = 0;
	for(unsigned int i = 0; i < node->wires.size(); i++)
		cost += node->wires[i].length * node->wires[i].capacity;
	cost += node->max_wires;
	return cost;
}

void CreateClientWire(Node* node, Client* client, const unsigned int length, const unsigned int capacity)
{
	Wire w = {length, capacity, client, node, nullptr};
	node->wires.push_back(w);
	client->wire = &node->wires[node->wires.size()-1];
}

void CreateNodeWire(Node* node, Node* to_node, const unsigned int length, const unsigned int capacity)
{
	node->wires.push_back({length, capacity, nullptr, node, to_node});
	to_node->wires.push_back({length, capacity, nullptr, to_node, node});
}

void OutputNodeProperties(Node node)
{
	std::cout<<"Node\n";
	std::cout<<"Index: "<<node.index<<'\n';
	std::cout<<"Max wires: "<<node.max_wires<<'\n';
	std::cout<<"Wires:\n";
	for(unsigned int i = 0; i < node.wires.size(); i++)
	{
		if(node.wires[i].client)
			std::cout<<"\tClient ";
		else
			std::cout<<"\tNode ";
		std::cout<< "index:"<<(node.wires[i].client?node.wires[i].client->index:node.wires[i].to_node->index);
		std::cout<< "\tLength:"<< node.wires[i].length;
		std::cout<< "\tCapacity:"<< node.wires[i].capacity<<'\n';
	}
	std::cout<<"Cost:\t"<< CalculateCost(&node)<<"\n\n";
}

Client* SearchClient(Query* q, Node* node, bool is_starting = true)
{
	static std::vector<INDEX> writed_index;
	if(q->index_from == 0 || q->index_to == 0)
		return nullptr;
	if(is_starting)
		writed_index.clear();
	for (size_t i = 0; i < node->related_clients.size(); i++)
	{
		for(size_t j = 0; j < writed_index.size(); j++)
			if(writed_index.at(j) == node->related_clients.at(i)->index)
				return nullptr; // Throw nothing
		if(node->related_clients.at(i)->index == q->index_to)
			return node->related_clients.at(i);
		writed_index.push_back(node->related_clients.at(i)->index);
	}
	for (size_t i = 0; i < node->wires.size(); i++)
	{
		if(!(node->wires.at(i).client))
		{
			auto cl = SearchClient(q,node, false);
			if(cl)
				return cl; 
		}
	}
	return nullptr;
}

void GiveClientIndex(std::vector<Client>* clients, unsigned int indexes)
{
	for(unsigned int i = 0; i < indexes; i++)
		(*clients)[i].index = i+1;
}

void GiveNodeIndex(std::vector<Node>* nodes, unsigned int indexes, unsigned int def_max_wires = 16)
{
	for(unsigned int i = 0; i < indexes; i++)
	{
		(*nodes)[i].index = i+1;
		(*nodes)[i].max_wires = def_max_wires;
	}
}

void PushClientsToNetwork(Network* n, std::vector<Client> clients)
{
	n->clients = clients;
}

void PushNodesToNetwork(Network* n, std::vector<Node> nodes)
{
	n->nodes = nodes;
}

void GiveNetworkIndex(Network* n, unsigned int def_max_wires = 16)
{
	GiveClientIndex(&n->clients, n->clients.size());
	GiveNodeIndex(&n->nodes, n->nodes.size(), def_max_wires);
}

void ConnectClients(Network* n, std::vector<std::pair<INDEX, INDEX>> client_node_list, unsigned int def_wire_len = 1, unsigned int def_wire_cap = 1)
{
	 // First - Client index, Second - Node inde
	static INDEX buf_index = 0;
	auto index_cl_comp = [](Client client) { return (client.index == buf_index); };
	auto index_nd_comp = [](Node node) { return (node.index == buf_index); };
	for(unsigned int i = 0; i < client_node_list.size(); i++)
	{
		buf_index = client_node_list[i].first;
		auto client = std::find_if(n->clients.begin(), n->clients.end(), index_cl_comp);
		buf_index = client_node_list[i].second;
		auto node = std::find_if(n->nodes.begin(), n->nodes.end(), index_nd_comp);
		CreateClientWire(&(*node), &(*client), def_wire_len, def_wire_cap);
	}
	for(unsigned int i = 0; i < n->nodes.size(); i++)
		GenerateRelatedNodes(&n->nodes[i]);
}

void ConnectNodes(Network* n, std::vector<std::pair<INDEX, INDEX>> node_node_list, unsigned int def_wire_len = 1, unsigned int def_wire_cap = 1)
{
	 // First - Node index, Second - Node inde
	static INDEX buf_index = 0;
	auto index_nd_comp = [](Node node) { return (node.index == buf_index); };
	for(unsigned int i = 0; i < node_node_list.size(); i++)
	{
		buf_index = node_node_list[i].first;
		auto node1 = std::find_if(n->nodes.begin(), n->nodes.end(), index_nd_comp);
		buf_index = node_node_list[i].second;
		auto node2 = std::find_if(n->nodes.begin(), n->nodes.end(), index_nd_comp);
		CreateNodeWire(&(*node1), &(*node2), def_wire_len, def_wire_cap);
	}
}

void ConnectClientsManually(Network* n, std::vector<std::array<INDEX,4>> client_node_list)
{
	static INDEX buf_index = 0;
	auto index_cl_comp = [](Client client) { return (client.index == buf_index); };
	auto index_nd_comp = [](Node node) { return (node.index == buf_index); };
	for(unsigned int i = 0; i < client_node_list.size(); i++)
	{
		buf_index = client_node_list[i][0];
		auto client = std::find_if(n->clients.begin(), n->clients.end(), index_cl_comp);
		buf_index = client_node_list[i][1];
		auto node = std::find_if(n->nodes.begin(), n->nodes.end(), index_nd_comp);
		CreateClientWire(&(*node), &(*client), client_node_list[i][2], client_node_list[i][3]);
	}	
}

void ConnectNodesManually(Network* n, std::vector<std::array<INDEX,4>> node_node_list)
{
	static INDEX buf_index = 0;
	auto index_nd_comp = [](Node node) { return (node.index == buf_index); };
	for(unsigned int i = 0; i < node_node_list.size(); i++)
	{
		buf_index = node_node_list[i][0];
		auto node1 = std::find_if(n->nodes.begin(), n->nodes.end(), index_nd_comp);
		buf_index = node_node_list[i][1];
		auto node2 = std::find_if(n->nodes.begin(), n->nodes.end(), index_nd_comp);
		CreateNodeWire(&(*node1), &(*node2), node_node_list[i][2], node_node_list[i][3]);
	}
	for(unsigned int i = 0; i < n->nodes.size(); i++)
		GenerateRelatedNodes(&n->nodes[i]);
}

int CalculateNetworkCost(Network* n)
{
	int cost = 0;
	for(int i = 0; i < n->nodes.size(); i++)
		cost += CalculateCost(&n->nodes[i]);
	return cost;
}

void OutputNetworkProperties(Network n)
{
	std::cout<<"Network\n";
	std::cout<<"Count of clients: " << n.clients.size()<<'\n';
	std::cout<<"Count of Nodes: " << n.nodes.size()<<'\n';
	std::cout<<"Nodes:\n";
	for(unsigned int i = 0; i < n.nodes.size(); i++)
	{
		std::cout << "\tNode ["<<n.nodes[i].index<<"]:\t";
		for(unsigned int j = 0; j < n.nodes[i].wires.size(); j++)
		{
			(n.nodes[i].wires[j].client)
				? std::cout <<n.nodes[i].wires[j].client->index
				: std::cout<<'['<< n.nodes[i].wires[j].to_node->index<<']';
			if(j + 1 != n.nodes[i].wires.size())
				std::cout<<",\t";
		}
		std::cout << '\n';
	}
	std::cout<<"Cost: "<<CalculateNetworkCost(&n)<<"\n\n";
}

void SaveNetworkSettings(Network n, const std::string dir)
{
	/*
	Definitions:
		*WireIndex -> Wire (indexated)
		*ClientIndex -> Client (indexated)
		*Wires -> Wire (all)
		*Signature = "For project Router" (string)
	Structure of file:
		Size of Network.clients
		->
			Network.clients.index = *ClientIndex (indexated)
			Network.clients.resources
			Network.clients.*WireIndex
		<-
		Size of Network.nodes
		->
			Network.nodes.index
			Network.nodes.max_wires
			Size of Network.nodes.wires
			->
				Network.nodes.wires.*WireIndex
			<-
			Size of Network.nodes.related_clients
			->
				Network.nodes.*ClientIndex
			<-
			
		<-
		Size of Wires
		->
			Wires (indexated) = WireIndex
			Wires.length
			Wires.capacity
			Wires.node.index
			If Wires.client != nullptr ->
				Wires.ClientIndex
			Else ->
				Wires.to_node.index
		<-
		
	*/
	// static std::ofstream fout;
	// static Node* node;
	// fout.open(dir);
	// fout << n.nodes.size()<<'\n';
	// for(unsigned int i = 0; i < n.nodes.size(); i++)
	// {
	// 	node = &n.nodes[i];
	// 	fout << '\t' << node->wires.size() << '\n';
	// 	for(unsigned int j = 0; j < node->wires.size(); j++)
	// 	{
	// 		fout << '\t';
	// 		fout << 
	// 	}
	// }
	// fout.close();
}

int main()
{
	std::vector<Client> clients(7);
	std::vector<Node> nodes(3);
	Network n;
	PushClientsToNetwork(&n, clients);
	PushNodesToNetwork(&n, nodes);
	GiveNetworkIndex(&n);
	ConnectClientsManually(&n, {{1,1,5,5},{2,1,4,3},{3,2,2,6},{4,3,8,6},{5,3,5,6},{6,3,7,7},{7,3,2,2}});
	ConnectNodesManually(&n, {{1,2,5,5},{2,3,5,5}});
	OutputNetworkProperties(n);
	OutputNodeProperties(n.nodes[0]);
	OutputNodeProperties(n.nodes[1]);
	OutputNodeProperties(n.nodes[2]);
	return 0;
	Query q;
	q.index_from = 4;
	// std::cout << "Index to: ";
	// std::cin >> q.index_to;
	q.index_to = 5;
	q.message = "Hahahahahaha!";
	Client* cl = SearchClient(&q, n.clients[3].wire->node);
	if(cl)
		std::cout<<"Success\n";
	else
		std::cout<<"Denied\n";
	return 0;	
}