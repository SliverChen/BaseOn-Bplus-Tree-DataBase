/***************************
 * Topic: 操作窗口的设置头文件
 * Author:Sliverchen
 * Create file date : 7 / 3
 * *************************/
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

class TextTable
{
public:
    enum class Alignment
    {
        LEFT,
        RIGHT
    };

    typedef std::vector<std::string> Row;
    TextTable(char horizontal = '-', char vertical = '|', char corner = '+')
    {
        _horizontal = horizontal;
        _vertical = vertical;
        _corner = corner;
    }

    //设置对齐
    void setAlignment(unsigned i, Alignment alignment)
    {
        _alignment[i] = alignment;
    }

    //获取对齐系数
    Alignment alignment(unsigned i) const
    {
        return _alignment[i];
    }

    //获取垂直边框
    char vertical() const
    {
        return _vertical;
    }

    //获取水平边框
    char horizontal() const
    {
        return _horizontal;
    }

    //添加属性字段
    void add(std::string const &content)
    {
        _current.push_back(content);
    }

    //处理行末
    void endOfRow()
    {
        _rows.push_back(_current);
        _current.assign(0, "");
    }

    //添加行（利用迭代器）
    template <typename Iterator>
    void addRow(Iterator begin, Iterator end)
    {
        for (auto i = begin; i != end; ++i)
            add(*i);
        endOfRow();
    }

    //添加行（利用容器）
    template <typename Container>
    void addRow(Container const &container)
    {
        addRow(container.begin(), container.end());
    }

    //获取行对象
    std::vector<Row> const &rows() const
    {
        return _rows;
    }

    //初始化
    void setup() const
    {
        determineWidths();
        setupAlignment();
    }

    //边框设计
    std::string ruler() const
    {
        std::string result;
        result += _corner;
        for (auto width = _width.begin(); width != _width.end(); ++width)
        {
            result += repeat(*width, _horizontal);
            result += _corner;
        }
        return result;
    }

    int width(unsigned i) const
    {
        return _width[i];
    }

private:
    char _horizontal;
    char _vertical;
    char _corner;
    Row _current;
    std::vector<Row> _rows;
    std::vector<unsigned> mutable _width;
    std::map<unsigned, Alignment> mutable _alignment;

    //以times个c组合得到的字符串
    static std::string repeat(unsigned times, char c)
    {
        std::string result;
        for (; times > 0; --times)
            result += c;

        return result;
    }

    //列数
    unsigned columns() const
    {
        return _rows[0].size();
    }

    //设置宽度
    void determineWidths() const
    {
        _width.assign(columns(), 0);
        for (auto rowIterator = _rows.begin(); rowIterator != _rows.end(); ++rowIterator)
        {
            Row const &row = *rowIterator;
            for (unsigned i = 0; i < row.size(); ++i)
            {
                _width[i] = _width[i] > row[i].size() ? _width[i] : row[i].size();
            }
        }
    }

    //对齐设置
    void setupAlignment() const
    {
        for (unsigned i = 0; i < columns(); ++i)
        {
            if (_alignment.find(i) == _alignment.end())
                _alignment[i] = Alignment::LEFT;
        }
    }
};

std::ostream &operator<<(std::ostream &stream, TextTable const &table)
{
    table.setup();
    stream << table.ruler() << "\n";
    for (auto rowIterator = table.rows().begin(); rowIterator != table.rows().end(); ++rowIterator)
    {
        TextTable::Row const &row = *rowIterator;
        stream << table.vertical();
        for (unsigned i = 0; i < row.size(); ++i)
        {
            auto alignment = table.alignment(i) == TextTable::Alignment::LEFT ? std::left : std::right;
            stream << std::setw(table.width(i)) << alignment << row[i];
            stream << table.vertical();
        }
        stream << "\n";
        stream << table.ruler() << "\n";
    }
    return stream;
}