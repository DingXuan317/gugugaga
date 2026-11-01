#include <iomanip>
#include <iostream>
#include <utility>

namespace
{
    bool isValidScore(double score)
    {
        return score >= 0.0 && score <= 100.0;
    }
}

Student::Student(std::string name, std::string studentId, int age)
    : name_(std::move(name)),
      studentId_(std::move(studentId)),
      age_(age),
      chineseScore_(0.0),
      mathScore_(0.0),
      englishScore_(0.0) {}

void Student::printInfo() const
{
    std::cout << "\n========== Student Information ==========\n";
    std::cout << std::left << std::setw(10) << "Name" << ": " << name_ << '\n';
    std::cout << std::left << std::setw(10) << "Student ID" << ": " << studentId_ << '\n';
    std::cout << std::left << std::setw(10) << "Age" << ": " << age_ << '\n';

    std::cout << std::fixed << std::setprecision(1);
    std::cout << std::left << std::setw(10) << "Chinese" << ": " << chineseScore_ << '\n';
    std::cout << std::left << std::setw(10) << "Math" << ": " << mathScore_ << '\n';
    std::cout << std::left << std::setw(10) << "English" << ": " << englishScore_ << '\n';

    std::cout << std::left << std::setw(10) << "Average" << ": "
              << std::setprecision(2) << averageScore() << '\n';
    std::cout << "=========================================\n";
}

bool Student::modifyScore(const std::string &subject, double newScore)
{
    if (!isValidScore(newScore))
    {
        return false;
    }

    if (subject == "Chinese")
    {
        chineseScore_ = newScore;
    }
    else if (subject == "Math")
    {
        mathScore_ = newScore;
    }
    else if (subject == "English")
    {
        englishScore_ = newScore;
    }
    else
    {
        return false;
    }

    return true;
}

const std::string &Student::getName() const
{
    return name_;
}

const std::string &Student::getStudentId() const
{
    return studentId_;
}

int Student::getAge() const
{
    return age_;
}

double Student::averageScore() const
{
    return (chineseScore_ + mathScore_ + englishScore_) / 3.0;
}
