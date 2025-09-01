#ifndef DATASET_H
#define DATASET_H
# include <math.h>
# include <stdio.h>
# include <stdlib.h>
# include <QString>
# include <QRegExp>
# include <vector>
# include <string>
using namespace std;
typedef vector<double> Data;
typedef vector<Data>   Matrix;

/**
 * @brief The Dataset class
 */
class Dataset
{
private:
    Matrix xpoint;
    Data patternClass;
    Data   ypoint;
public:
     /**
     * @brief Dataset, the Default constructor.
     */
    Dataset();
    /**
     * @brief Dataset, reads data from filename.
     * @param filename
     * @param format, the used format type: data,csv,arff
     */
    Dataset(QString filename,QString format="data");

    /**
     * @brief setData, sets the data values (x,y)
     * @param x,  the patterns
     * @param y,  the labels
     */
    void    setData(Matrix &x,Data &y);
    /**
     * @brief addPoint Adds a new point to the dataset.
     * @param x
     * @param y
     */
    void    addPoint(Data &x,double y);

    /**
     * @brief saveData Saves dataset to filename.
     * @param filename
     */
    void    saveData(char *filename);

    /**
     * @brief getXPoint
     * @param @pos
     * @return the pattern in position pos.
     */
    Data    getXPoint(int pos) const;

    /**
     * @brief getYPoint
     * @param pos
     * @return the label in position pos.
     */
    double  getYPoint(int pos) const;

    /**
     * @brief maxx
     * @param pos
     * @return the maximum value for feature pos.
     */
    double  maxx(int pos) const;

    /**
     * @brief minx
     * @param pos
     * @return the minimum value for feature pos.
     */
    double  minx(int pos) const;

    /**
     * @brief miny
     * @return the minimum label.
     */
    double  miny() const;

    /**
     * @brief maxy
     * @return the maximum label.
     */
    double  maxy() const;

    /**
     * @brief meanx
     * @param pos
     * @return the median value for feature pos.
     */
    double  meanx(int pos) const;

    /**
     * @brief stdx
     * @param pos
     * @return the standard deviation for feature pos.
     */
    double  stdx(int pos) const;

    /**
     * @brief meany
     * @return the median value for labels.
     */
    double  meany() const;

    /**
     * @brief stdy
     * @return the standard deviation for labels.
     */
    double  stdy() const;

    /**
     * @brief count
     * @return the number of features.
     */
    int     count() const;

    /**
     * @brief dimension
     * @return the pattern dimension.
     */
    int     dimension() const;

    /**
     * @brief normalizeMinMax Normalizes dataset.
     */
    void    normalizeMinMax();

    /**
     * @brief getPatternClass
     * @return the list of distinct classes.
     */
    Data    getPatternClass();

    /**
     * @brief getClass
     * @param pos
     * @return the nearest class for pattern in position pos.
     */
    double  getClass(int pos) const;

    /**
     * @brief getClass
     * @param value
     * @return the corresponding class for value (nearest class).
     */
    double          getClass(double value)const;
    /**
     * @brief getAllXpoint
     * @return a list with all patterns.
     */
    vector<Data>    getAllXpoint();
    /**
     * @brief getAllYpoint
     * @return a list with all labels.
     */
    Data            getAllYpoint();
    /**
     * @brief clearPoints Removes all patterns and labels.
     */
    void            clearPoints();
    /**
     * @brief makePatternClass Creates the vector with the distinct classes of the problem.
     */
    void makePatternClass();

    /**
     * @brief setPoint Sets the pattern and the label for position pos.
     * @param pos
     * @param x
     * @param y
     */
    void    setPoint(int pos,Data &x,double y);
    int     nearestClassIndex(double value);
    Data    getAllYPoints() ;
    ~Dataset();
};

#endif // DATASET_H
