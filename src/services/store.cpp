#include "services/logger.hpp"
#include "services/store.hpp"

Store::Store(int size)
{
  log_trace("Creating shared memory container");
  this->region = boost::interprocess::anonymous_shared_memory(size);
  this->state_region = boost::interprocess::anonymous_shared_memory(sizeof(char));
}

void Store::flag()
{
  std::memset(this->state_region.get_address(), this->FLAG_DATA, this->state_region.get_size());
}

void Store::unflag()
{
  std::memset(this->state_region.get_address(), this->FLAG_NO_DATA, this->state_region.get_size());
}

bool Store::check()
{
  char test;
  std::memcpy(&test, this->state_region.get_address(), this->state_region.get_size());
  return this->FLAG_DATA == test;
}

char &Store::get(char &d)
{
  std::memcpy(&d, this->region.get_address(), this->region.get_size());
  this->unflag();
  log_trace("Fetched: \""+ std::to_string(d) +"\"");
  return d;
}

void Store::set(char val)
{
  log_trace("Storing: \""+ std::to_string(val) +"\"");
  std::memset(this->region.get_address(), val, this->region.get_size());
  this->flag();
}

std::string Store::get_string()
{
  std::string s((char *) this->region.get_address());
  this->unflag();
  log_trace("Fetched: \""+ s +"\"");
  return s;
}

std::string &Store::get_string(std::string& s)
{
  s.clear();
  s.append((char *) this->region.get_address());
  this->unflag();
  log_trace("Fetched: \""+ s +"\"");
  return s;
}

void Store::set_string(const std::string& s)
{
  log_trace("Storing: \""+ s +"\"");
  std::memcpy(this->region.get_address(), s.c_str(), this->region.get_size());
  this->flag();
}
