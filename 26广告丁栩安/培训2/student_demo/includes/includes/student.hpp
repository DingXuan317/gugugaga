#pragma once

#include <string>

class Student
{
public:
    Student(std::string name, std::string studentId, int age);

    void printInfo() const;
    bool modifyScore(const std::string &subject, double newScore);

    const std::string &getName() const;
    const std::string &getStudentId() const;
    int getAge() const;

    double averageScore() const;

private:
    std::string name_;
    std::string studentId_;
    int age_;
    double chineseScore_;
    double mathScore_;
    double englishScore_;
};
