#include "inverse_capability_map/InverseCapabilityOcTreeNode.h"
#include <ros/ros.h>
#include <algorithm>
//#include <cmath>

/**************************************************************************************************
 *
 * 										InverseCapability
 *
 *************************************************************************************************/

InverseCapability::InverseCapability()
{
}

InverseCapability::InverseCapability(const std::map<double, double> &thetas)
{
	thetas_ = thetas;
}

double InverseCapability::getThetaPercent(const double theta)
{
	std::map<double, double>::iterator it;
	it = thetas_.find(theta);
	if (it != thetas_.end())
		return it->second;
	else
		return 0.0;
}

bool InverseCapability::operator==(const InverseCapability &other) const
{
    return thetas_.size() == other.thetas_.size()
        && std::equal(thetas_.begin(), thetas_.end(),
                      other.thetas_.begin());
}

bool InverseCapability::operator!=(const InverseCapability &other) const
{
    return !(this->thetas_ == other.thetas_);
}

InverseCapability InverseCapability::operator+(const InverseCapability &other) const
{
	InverseCapability result, copy_other;
	copy_other = other;
	std::map<double, double>::const_iterator it;
	std::map<double, double>::iterator other_it;
	for (it = this->thetas_.begin(); it != this->thetas_.end(); it++)
	{
		other_it = copy_other.thetas_.find(it->first);
		// key not found in other, store in result inverse capability
		if (other_it == copy_other.thetas_.end())
			result.setThetaPercent(*it);
		else
		{
			// if match found in other, add percentages of both and add to result
			// further, remove match from other, since its match was treated
			ROS_ASSERT(it->first == other_it->first);
			double percent = it->second + other_it->second;
			result.setThetaPercent(std::make_pair(it->first, percent));
			copy_other.thetas_.erase(other_it);
		}
	}

	// add (theta, percent) pairs from other which don't have a match
	for (it = copy_other.thetas_.begin(); it != copy_other.thetas_.end(); it++)
	{
		result.setThetaPercent(*it);
	}

	ROS_ASSERT(result.thetas_.size() <= this->thetas_.size() + other.thetas_.size());

	return result;
}

InverseCapability InverseCapability::operator&(const InverseCapability &other) const
{
	InverseCapability result, copy_other;
	copy_other = other;
	std::map<double, double>::const_iterator it;
	std::map<double, double>::iterator other_it;
	for (it = this->thetas_.begin(); it != this->thetas_.end(); it++)
	{
		other_it = copy_other.thetas_.find(it->first);
		// key not found in other, store in result inverse capability
		if (other_it == copy_other.thetas_.end())
			result.setThetaPercent(*it);
		else
		{
			// if match found in other, take highest percentage of both and store in result
			// further, remove match from other, since its match was treated
			ROS_ASSERT(it->first == other_it->first);
			double percent;
			if (it->second < other_it->second)
				percent = other_it->second;
			else
				percent = it->second;
			result.setThetaPercent(std::make_pair(it->first, percent));
			copy_other.thetas_.erase(other_it);
		}
	}

	// add (theta, percent) pairs from other which don't have a match
	for (it = copy_other.thetas_.begin(); it != copy_other.thetas_.end(); it++)
	{
		result.setThetaPercent(*it);
	}

	ROS_ASSERT(result.thetas_.size() <= this->thetas_.size() + other.thetas_.size());

	return result;
}

void InverseCapability::normalize(const double& value)
{
	std::map<double, double>::iterator it;
	for (it = this->thetas_.begin(); it != this->thetas_.end(); it++)
		it->second = it->second / value;
}

const std::pair<double, double> & InverseCapability::getMaxThetaPercent()
{
	std::map<double, double>::iterator it;
	it = std::max_element(thetas_.begin(), thetas_.end(), LessThanSecond());
	return *it;
}

std::map<double, double> InverseCapability::getThetasWithMinPercent(double minPercent) const
{
	std::map<double, double> ret;
	std::map<double, double>::const_iterator it;
	for (it = thetas_.begin(); it != thetas_.end(); it++)
	{
		if (it->second > minPercent)
			ret.insert(*it);
	}
	return ret;
}

/**************************************************************************************************
 *
 * 									InverseCapabilityOcTreeNode
 *
 *************************************************************************************************/

InverseCapabilityOcTreeNode::InverseCapabilityOcTreeNode()
{
}

InverseCapabilityOcTreeNode::InverseCapabilityOcTreeNode(InverseCapability inv_capa)
	: OcTreeDataNode<InverseCapability>(inv_capa)
{
}

InverseCapabilityOcTreeNode::InverseCapabilityOcTreeNode(const InverseCapabilityOcTreeNode &rhs)
	: OcTreeDataNode<InverseCapability>(rhs)
{
}

InverseCapabilityOcTreeNode::~InverseCapabilityOcTreeNode()
{
}

bool InverseCapabilityOcTreeNode::createChild(unsigned int i)
{
    if (children == NULL)
    {
        allocChildren();
    }
    assert (children[i] == NULL);
    children[i] = new InverseCapabilityOcTreeNode();
    return true;
}

std::ostream& InverseCapabilityOcTreeNode::writeValue(std::ostream &s) const
{
    // 1 bit for each children; 0: empty, 1: allocated
    std::bitset<8> children;
    for (unsigned int i = 0; i < 8; ++i)
    {
        if (childExists(i))
        {
          children[i] = 1;
        }
        else
        {
          children[i] = 0;
        }
    }
    char children_char = (char)children.to_ulong();

    // buffer inverse capability data
    std::map<double, double> thetas = value.getThetasPercent();
    unsigned int size = thetas.size();

    // write node data
    s.write((const char*)&size, sizeof(unsigned int));
    std::map<double, double>::iterator it;
    double theta, percent;
    for (it = thetas.begin(); it != thetas.end(); it++)
    {
    	theta = it->first;
    	percent = it->second;
    	s.write((const char*)&theta  , sizeof(double));
    	s.write((const char*)&percent, sizeof(double));
    }
    s.write((char*)&children_char, sizeof(char)); // child existence

    // write existing children
    for (unsigned int i = 0; i < 8; ++i)
    {
        if (children[i] == 1)
        {
            this->getChild(i)->writeValue(s);
        }
    }
    return s;
}

std::istream& InverseCapabilityOcTreeNode::readValue(std::istream &s)
{
    // buffer for capabilities' data
    unsigned int size;
    std::map<double, double> thetas;
    char children_char;

    // read node data
    s.read((char*)&size, sizeof(unsigned int));

    double theta, percent;
    for (unsigned int i = 0; i < size; i++)
    {
    	s.read((char*)&theta  , sizeof(double));
    	s.read((char*)&percent, sizeof(double));
    	thetas.insert(std::make_pair(theta, percent));
    }
    s.read((char*)&children_char, sizeof(char)); // child existence

    // insert buffered data into node
    value.setThetasPercent(thetas);

    // read existing children
    std::bitset<8> children ((unsigned long long)children_char);
    for (unsigned int i = 0; i < 8; ++i)
    {
        if (children[i] == 1)
        {
            createChild(i);
            getChild(i)->readValue(s);
        }
    }
    return s;
}


