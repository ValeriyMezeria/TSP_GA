#include "tsp.h"

#include <fstream>
#include <string>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <random>

namespace {

std::string ltrim(const std::string& str)
{
  const std::string pattern = " \f\n\r\t\v";
  return str.substr(str.find_first_not_of(pattern));
}

std::string rtrim(const std::string& str)
{
  const std::string pattern = " \f\n\r\t\v";
  return str.substr(0,str.find_last_not_of(pattern) + 1);
}

std::string trim(const std::string& str)
{
  return ltrim(rtrim(str));
}

int rand(int a, int b)
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist(a, b);

    return dist(mt);
}

TYPE str2type(const std::string& str)
{
    if (str == "TSP")
        return TYPE::TSP;
    else if (str == "ATSP")
        return TYPE::ATSP;
    else
        return TYPE::NONE;
}

EDGE_WEIGHT_TYPE str2edgeWeightType(const std::string& str)
{
    if (str == "EXPLICIT")
        return EDGE_WEIGHT_TYPE::EXPLICIT;
    else if (str == "EUC_2D")
        return EDGE_WEIGHT_TYPE::EUC_2D;
    else if (str == "ATT")
        return EDGE_WEIGHT_TYPE::ATT;
    else
        return EDGE_WEIGHT_TYPE::NONE;
}

EDGE_WEIGHT_FORMAT str2edgeWeightFormat(const std::string& str)
{
    if (str == "FULL_MATRIX")
        return EDGE_WEIGHT_FORMAT::FULL_MATRIX;
    else
        return EDGE_WEIGHT_FORMAT::NONE;
}

SECTION str2section(const std::string& str)
{
    if (str == "EDGE_WEIGHT_SECTION")
        return SECTION::EDGE_WEIGHT_SECTION;
    else if (str == "NODE_COORD_SECTION")
        return SECTION::NODE_COORD_SECTION;
    else
        return SECTION::NONE;
}

}

bool TSP::readFromFile(const std::string& filename)
{
    std::ifstream ifs(filename.c_str());

    if (!ifs.is_open())
        return false;

    std::string tmp;

    while (getline(ifs, tmp))
    {
        std::string val;

        if (parseParam(tmp, "NAME", val))
            m_name = val;
        else if (parseParam(tmp, "COMMENT", val))
            m_comment = val;
        else if (parseParam(tmp, "DIMENSION", val))
            m_size = std::stoi(val);
        else if (parseParam(tmp, "EDGE_WEIGHT_TYPE", val))
            m_edgeWeightType = str2edgeWeightType(val);
        else if (parseParam(tmp, "TYPE", val))
            m_type = str2type(val);
        else if (parseParam(tmp, "EDGE_WEIGHT_FORMAT", val))
            m_edgeWeightFormat = str2edgeWeightFormat(val);


        if (isEOF(tmp))
            break;

        if (isSection(tmp))
        {
            if (str2section(tmp) == SECTION::EDGE_WEIGHT_SECTION)
                readMatrix(ifs);
            else if (str2section(tmp) == SECTION::NODE_COORD_SECTION)
                readNodeCoord(ifs);
        }
    }

    return true;
}

bool TSP::readInitial(const std::string& filename)
{
    std::ifstream ifs(filename);

    if (!ifs.is_open())
        return false;

    while(ifs.good())
    {
        m_population.push_back(std::vector<double>());
        for (size_t i = 0; i < m_size; i++)
        {
            double tmp;
            ifs >> tmp;

            if (!ifs.good())
            {
                m_population.pop_back();
                if (m_population.empty())
                    return false;
                else
                    return true;
            }

            m_population.back().push_back(tmp);
        }
    }

    return true;
}

void TSP::solve(CROSSOVER_SELECTION cross,bool verbose)
{
    std::chrono::time_point<std::chrono::system_clock> start, end;
    int time = 0;

    start = std::chrono::system_clock::now();

    sortAndBest();

    if (verbose)
    {
        std::cout << "Best initial: " << m_record << std::endl;
    }

    GA(cross);

    end = std::chrono::system_clock::now();
    time = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();

    if (verbose)
    {
        std::cout << "Elapsed time: " << time << " ms" << std::endl;
        std::cout << "Record lenght: " << m_record << std::endl;
        std::cout << "Path: ";
        for (auto i : m_path) std::cout << i << " ";
        std::cout << std::endl << std::endl;
    }
}

bool TSP::isSection(const std::string& str)
{
    return str2section(str) != SECTION::NONE;
}

bool TSP::isEOF(const std::string& str)
{
    return str == "EOF";
}

bool TSP::parseSection(const std::string& str)
{
    return str2section(str) != SECTION::NONE;
}

bool TSP::parseParam(const std::string& str, const std::string& name, std::string& result)
{
    size_t pos;

    if (str.find(name) == std::string::npos)
        return false;

    if ((pos = str.find(':')) == std::string::npos)
        return false;

    std::string val(str, pos + 1, str.size());
    result = trim(val);

    return true;
}

bool TSP::readMatrix(std::ifstream& ifs)
{
    for (size_t i = 0; i < m_size; i++)
    {
        m_matrix.push_back(std::vector<double>());
        for (size_t j = 0; j < m_size; j++)
        {
            double tmp;
            ifs >> tmp;

            if (!ifs)
                return false;

            m_matrix.back().push_back(tmp);
        }
    }

    return true;
}

bool TSP::readNodeCoord(std::ifstream& ifs)
{
    std::vector<std::pair<double, double> > coord;
    int n;

    for (size_t i = 0; i < m_size; i++)
    {
        std::pair<double, double> tmp;
        ifs >> n;
        ifs >> tmp.first;
        ifs >> tmp.second;

        coord.push_back(tmp);
    }

    for (size_t i = 0; i < m_size; i++)
    {
        m_matrix.push_back(std::vector<double>());
        for (size_t j = 0; j < m_size; j++)
            if (i == j)
                m_matrix.back().push_back(INF);
            else
                m_matrix.back().push_back(dist(coord[i], coord[j]));
    }

    return true;
}

double TSP::dist(const std::pair<double, double>& a, const std::pair<double, double>& b)
{
    if (m_edgeWeightType == EDGE_WEIGHT_TYPE::ATT)
    {
        double r = sqrt(((b.first - a.first) * (b.first - a.first) + (b.second - a.second) * (b.second - a.second)) / 10.0);
        return round(r) < r ? round(r) + 1.0 : round(r);
    }

    return sqrt((b.first - a.first) * (b.first - a.first) + (b.second - a.second) * (b.second - a.second));
}

double TSP::getLenght(const std::vector<double>& path)
{
    double result = 0;

    for (size_t i = 0; i < m_size - 1; i++)
        result += m_matrix[path[i]][path[i + 1]];

    result += m_matrix[path[m_size - 1]][path[0]];

    return result;
}

double TSP::getFitness(const std::vector<double>& path)
{
    double sum = 0.0;
    for (auto i : m_population) sum += getLenght(i);
    return 1.0 - getLenght(path) / sum;
}

void TSP::sortAndBest()
{
    auto tmp(m_population);
    std::sort(tmp.begin(), tmp.end(), [this](const std::vector<double>& a, const std::vector<double>& b)
                                      { return getFitness(a) > getFitness(b); });
    m_population = tmp;
    m_path = m_population[0];
    m_record = getLenght(m_path);
}

void TSP::GA(CROSSOVER_SELECTION cross)
{
    for (size_t i = 0; i < ITERATIONS; i++)
    {
        crossover(selectionForCrossover(cross));

        sortAndBest();

        while (m_population.size() > m_size)
            m_population.pop_back();
    }
}

std::vector<double> TSP::PMX(const std::vector<double>& p1, const std::vector<double>& p2, size_t k)
{
    std::vector<double> res;

    for (size_t i = 0; i < p1.size(); i++)
        if (i <= k)
            res.push_back(p1[i]);
        else
            res.push_back(-1);

    for (size_t i = 0; i <= k; i++)
    {
        if (std::find(res.begin(), res.end(), p2[i]) == res.end())
        {
            size_t pos = i;
            double val = p2[i];

            while (pos <= k)
                pos = std::distance(p2.begin(), std::find(p2.begin(), p2.end(), p1[pos]));

            res[pos] = val;
        }
    }

    for (size_t i = 0; i < res.size(); i++)
        if (res[i] == -1)
            res[i] = p2[i];

    return res;
}

std::vector<std::vector<double>> TSP::selectionForCrossover(CROSSOVER_SELECTION cross)
{
    if (cross == CROSSOVER_SELECTION::TOURNAMENT)
        return tournament();
    else
        return proportional();
}

std::vector<std::vector<double>> TSP::tournament()
{
    std::vector<std::vector<double>> result;

    for (size_t i = 0; i < m_population.size() / SELECTION_PART; i++)
    {
        int a = rand(0, m_population.size()-1);
        int b = rand(0, m_population.size()-1);

        while (a == b)
            b = rand(0, m_population.size()-1);

        if (getFitness(m_population[a]) > getFitness(m_population[b]))
            result.push_back(m_population[a]);
        else
            result.push_back(m_population[b]);
    }

    return result;
}

std::vector<std::vector<double>> TSP::proportional()
{
    std::vector<std::vector<double>> result;

    double minFitness = getFitness(m_population[m_population.size() - 1]);
    double maxFitness = getFitness(m_population[0]);

    for (size_t i = 0; i < m_population.size(); i++)
    {
        if (result.size() == m_population.size() / SELECTION_PART)
            break;

        if (i < ELITE)
        {
            result.push_back(m_population[i]);
            continue;
        }

        double die = rand(1, 100) / 100.0;
        double normalizedFitness = (getFitness(m_population[i]) - minFitness) / (maxFitness - minFitness);

        if (normalizedFitness > die)
            result.push_back(m_population[i]);
    }

    return result;
}

std::vector<double> TSP::mutation(const std::vector<double>& individual)
{
    std::vector<double> result(individual);

    for (size_t i = 0; i < MUTATION_SIZE; i++)
    {
        double die = rand(1, 100) / 100.0;
        if (die < MUTATION_PROBABILITY)
        {
            int a = rand(0, individual.size() - 1);
            int b = rand(0, individual.size() - 1);

            while (a == b)
                b = rand(0, individual.size() - 1);

            std::swap(result[a], result[b]);
        }
    }

    return result;
}

void TSP::crossover(const std::vector<std::vector<double>>& population)
{
    std::vector<std::vector<double>> result;

    int k = rand(1, population[0].size() - 2);

    for (size_t i = 0; i < population.size() - 1; i++)
    {
        m_population.push_back(mutation(PMX(population[i], population[i+1], k)));
        m_population.push_back(mutation(PMX(population[i+1], population[i], k)));
    }
}
