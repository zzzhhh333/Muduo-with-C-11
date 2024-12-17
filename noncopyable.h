#pragma once
/*
*派生类继承该类后，拷贝和赋值被禁用
*其他方面不受影响
*/




class noncopyable
{
public:
    noncopyable(const noncopyable&)=delete;
    noncopyable operator=(const noncopyable&)=delete;
protected:
    noncopyable()=default;
    ~noncopyable()=default;
};