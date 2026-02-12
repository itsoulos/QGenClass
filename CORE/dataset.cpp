#include "dataset.h"
# include <QStringList>
# include <QFile>
# include <QTextStream>
Dataset::Dataset()
{
    xpoint.resize(0);
    ypoint.resize(0);
}



double Dataset::distance(const std::vector<double>& a, const std::vector<double>& b)
{
    double sum = 0.0;
    for (size_t i = 0; i < a.size(); i++) {
        sum += std::pow(a[i] - b[i], 2);
    }
    return std::sqrt(sum);
}

std::vector<int> Dataset::kNearest(
    const std::vector<Sample>& minority,
    int index,
    int k)
{
    std::vector<std::pair<double, int>> distances;

    for (size_t i = 0; i < minority.size(); i++) {
        if (i == index) continue;
        double d = distance(minority[index].features,
                            minority[i].features);
        distances.push_back({d, i});
    }

    std::sort(distances.begin(), distances.end());

    std::vector<int> neighbors;
    for (int i = 0; i < k && i < distances.size(); i++) {
        neighbors.push_back(distances[i].second);
    }

    return neighbors;
}

std::vector<Sample> Dataset::applySMOTE(
    const std::vector<Sample>& data,
    int k
    )
{
    std::map<double, std::vector<Sample>> classes;

    for (const auto& s : data) {
        classes[s.label].push_back(s);
    }

    // εύρεση πλειοψηφικής κλάσης
    int maxCount = 0;
    for (auto& c : classes) {
        maxCount = std::max(maxCount, (int)c.second.size());
    }

    std::vector<Sample> balanced = data;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(0.0, 1.0);

    for (auto& [label, samples] : classes) {
        if (samples.size() == maxCount) continue;

        int needed = maxCount - samples.size();
        int idx = 0;

        while (needed-- > 0) {
            const Sample& x = samples[idx % samples.size()];
            auto neighbors = kNearest(samples, idx % samples.size(), k);

            int nIndex = neighbors[gen() % neighbors.size()];
            const Sample& n = samples[nIndex];

            Sample synthetic;
            synthetic.label = label;

            for (size_t f = 0; f < x.features.size(); f++) {
                double gap = dist(gen);
                double val = x.features[f] +
                             gap * (n.features[f] - x.features[f]);
                synthetic.features.push_back(val);
            }

            balanced.push_back(synthetic);
            idx++;
        }
    }

    return balanced;
}

double euclideanDistance(const vector<double>& a, const vector<double>& b) {
    double sum = 0.0;
    for (size_t i = 0; i < a.size(); i++) {
        double diff = a[i] - b[i];
        sum += diff * diff;
    }
    return sqrt(sum);
}
void    Dataset::makeSmote(int k)
{
    vector<Sample> dataset = getSamples();
    vector<Sample> dataset2=applySMOTE(dataset,k);
    clearPoints();
    for(unsigned int i=0;i<dataset2.size();i++)
    {
        xpoint.push_back(dataset2[i].features);
        ypoint.push_back(dataset2[i].label);
    }
}


vector<Sample> Dataset::getSamples()
{
    vector<Sample> p;
    for(int i=0;i<count();i++)
    {
        Sample t;
        t.features=xpoint[i];
        t.label=ypoint[i];
        p.push_back(t);
    }
    return p;
}


vector<int> Dataset::kNearestNeighbors(const vector<Sample>& samples, int index, int k)
{
    vector<pair<double,int>> distances;
    for (size_t i = 0; i < samples.size(); i++) {
        if ((int)i == index) continue;
        double dist = euclideanDistance(samples[index].features, samples[i].features);
        distances.push_back({dist, (int)i});
    }
    sort(distances.begin(), distances.end());
    vector<int> neighbors;
    for (int i = 0; i < k && i < (int)distances.size(); i++) {
        neighbors.push_back(distances[i].second);
    }
    return neighbors;
}

Data    Dataset::getAllYPoints()
{
    return ypoint;
}

void Dataset::addPoint(Data &x,double y)
{
    xpoint.push_back(x);
    ypoint.push_back(y);
}
//arff
Dataset::Dataset(QString filename,QString format)
{
    if(format == "data")
    {
        FILE *fp=fopen(filename.toStdString().c_str(),"r");
        if(!fp) return;
        int d,count;
        fscanf(fp,"%d",&d);
        fscanf(fp,"%d",&count);
        ypoint.resize(count);

        xpoint.resize(count);
        for(int i=0;i<count;i++)
        {
            xpoint[i].resize(d);
            for(int j=0;j<d;j++)
                fscanf(fp,"%lf",&xpoint[i][j]);
            fscanf(fp,"%lf",&ypoint[i]);
        }
        fclose(fp);
    }
    else
    if(format=="csv")
    {
        QFile fp(filename);
        if(!fp.open(QIODevice::ReadOnly | QIODevice::Text)) return ;
        QTextStream st(&fp);
        while(!st.atEnd())
        {
            QString line=st.readLine();
            if(line.size()<=1) continue;
            QStringList list = line.split(",");
            Data xx;
            for(int i=0;i<list.size()-1;i++)
                xx.push_back(list[i].toDouble());
            double yy=list.last().toDouble();
            xpoint.push_back(xx);
            ypoint.push_back(yy);
        }
        fp.close();
    }
    else
    if(format=="arff")
    {
        /*Data ypoint;
        QStringList classNames;
        int nattributes=0;
        QFile fp(filename);
        if(!fp.open(QIODevice::ReadOnly | QIODevice::Text)) return ;
        QTextStream st(&fp);
        while(!st.atEnd())
        {
            QString line=st.readLine();
            if(line.size()<=1) continue;
            if(line.startsWith("@data") || line.startsWith("@relation")) continue;
            if(line.startsWith("@attribute"))
            {
                QStringList list = line.split(QRegExp("[\r\n\t ]+"), QString::SkipEmptyParts);
                if(list[1]!="class")
                nattributes++;
                else
                {
                    QString cstring=list[2];
                    cstring=cstring.mid(1,cstring.size()-2);
                    classNames=cstring.split(",");
                }
            }
            else
            {
                QStringList list = line.split(QRegExp("[\r\n\t ]+"), QString::SkipEmptyParts);
                if(list.size()<=2)
                {
                    continue;
                }
                Data xx;
                for(int i=0;i<list.size()-1;i++)
                   xx.push_back(list[i].toDouble());

                double yy=classNames.indexOf(list[list.size()-1]);

                xpoint.push_back(xx);
                ypoint.push_back(yy);
            }
        }
        fp.close();*/
    }
    makePatternClass();
}

/** euresi monadikon katigorion **/
void    Dataset::makePatternClass()
{


    patternClass.resize(0);
    for(unsigned int i=0;i<ypoint.size();i++)
    {
        bool found=false;
        for(unsigned int j=0;j<patternClass.size();j++)
        {
            //epeidi oi dekadikoi den anaparistantai panta akribos
            if(fabs(patternClass[j]-ypoint[i])<1e-5)
            {
                found=true;
                break;
            }
        }
        if(!found) patternClass.push_back(ypoint[i]);
    }
}

void    Dataset::setData(Matrix &x,Data &y)
{
    if(x.size()!=y.size()) return;
    xpoint.resize(x.size());
    ypoint.resize(y.size());
    int d=x[0].size();
    for(unsigned int i=0;i<x.size();i++)
    {
        xpoint[i].resize(d);
        for(int j=0;j<d;j++)
            xpoint[i][j]=x[i][j];
        ypoint[i]=y[i];
    }
    makePatternClass();
}

void    Dataset::saveData(char *filename)
{
    FILE *fp=fopen(filename,"w");
    if(!fp) return;
    fprintf(fp,"%d\n%d\n",dimension(),count());
    for(int i=0;i<count();i++)
    {
        for(int j=0;j<dimension();j++)
            fprintf(fp,"%lf ",xpoint[i][j]);
        fprintf(fp,"%lf\n",ypoint[i]);
    }
    fclose(fp);
}


//epistrefei ena pattern se mia thesi
Data    Dataset::getXPoint(int pos) const
{
    if(pos<0 || pos>=count()) return xpoint[0];
    return xpoint[pos];
}


//epistrefei exodo
double  Dataset::getYPoint(int pos) const
{
    if(pos<0 || pos>=count()) return -1;
    return ypoint[pos];
}

//pos = > number of feature
double  Dataset::meanx(int pos) const
{
    if(pos<0 || pos>=count()) return -1;
    double s=0.0;
    for(unsigned int i=0;i<xpoint.size();i++)
        s+=xpoint[i][pos];
    return s/xpoint.size();
}

double  Dataset::stdx(int pos) const
{
    if(pos<0 || pos>=count()) return -1;
    double s=0.0;
    double mx=meanx(pos);
    for(unsigned int i=0;i<xpoint.size();i++)
        s+=(xpoint[i][pos]-mx)*(xpoint[i][pos]-mx);
    return sqrt(1.0/xpoint.size() * s);
}


//mesos oros tis exodou
double  Dataset::meany() const
{
    double s=0.0;
    for(unsigned int i=0;i<ypoint.size();i++)
        s+=ypoint[i];
    return s/ypoint.size();
}

double  Dataset::stdy() const
{
    double my=meany();
    double s=0.0;
    for(unsigned int i=0;i<ypoint.size();i++)
        s+=(ypoint[i]-my)*(ypoint[i]-my);
    return sqrt(1.0/ypoint.size() * s);
}


//=> pos number of feature
double  Dataset::maxx(int pos) const
{
    double m=xpoint[0][pos];
    for(unsigned int i=0;i<xpoint.size();i++)
        if(xpoint[i][pos]>m) m=xpoint[i][pos];
    return m;
}

double  Dataset::minx(int pos) const
{
    double m=xpoint[0][pos];
    for(unsigned int i=0;i<xpoint.size();i++)
        if(xpoint[i][pos]<m) m=xpoint[i][pos];
    return m;
}

double  Dataset::miny() const
{
    double m=ypoint[0];
    for(unsigned int i=0;i<ypoint.size();i++)
        if(ypoint[i]<m) m=ypoint[i];
    return m;
}

double  Dataset::maxy() const
{
    double m=ypoint[0];
    for(unsigned int i=0;i<ypoint.size();i++)
        if(ypoint[i]>m) m=ypoint[i];
    return m;
}

int     Dataset::count() const
{
    return ypoint.size();
}


//epistrefei stiles
int     Dataset::dimension() const
{
    if(xpoint.size()==0)
        return 0;
    return xpoint[0].size();
}


Data    Dataset::getPatternClass()
{
    return patternClass;
}

void    Dataset::normalizeMinMax()
{
    Data MinX,MaxX;
    double MinY,MaxY;

    //ypologizo megisto elaxisto ana feature
    MinX.resize(dimension());
    MaxX.resize(dimension());
    for(int i=0;i<dimension();i++)
    {
        MinX[i]=minx(i);
        MaxX[i]=maxx(i);
    }
    MinY=miny();
    MaxY=maxy();
    for(int i=0;i<count();i++)
    {
        for(int j=0;j<dimension();j++)
        {
            xpoint[i][j]=(MaxX[j]-xpoint[i][j])/(MaxX[j]-MinX[j]);
        }
        ypoint[i]=(MaxY-ypoint[i])/(MaxY-MinY);
    }
}

double  Dataset::getClass(double value) const
{
    int imin =-1;
    double dmin=1e+100;
    for(unsigned int i=0;i<patternClass.size();i++)
        if(fabs(value-patternClass[i])<dmin)
        {
            dmin = fabs(value - patternClass[i]);
            imin = i;
        }

    return patternClass[imin];
}

vector<Data>    Dataset::getAllXpoint()
{
    return xpoint;
}

Data Dataset::getAllYpoint()
{
    return ypoint;
}


void    Dataset::clearPoints()
{
	auto pos1 = xpoint.begin();
	auto last1 = xpoint.end();
	xpoint.erase(pos1,last1);
	auto pos2=ypoint.begin();
	auto end2=ypoint.end();
	ypoint.erase(pos2,end2);
}

double  Dataset::getClass(int pos) const
{
    double y=ypoint[pos];
    return getClass(y);
}

int Dataset::nearestClassIndex(double value)
{
     double f= getClass(value);
     for(unsigned int i=0;i<patternClass.size();i++)
     {
         if(fabs(patternClass[i]-f)<1e-5) return i;
     }
     return 0;
}
void    Dataset::setPoint(int pos,Data &x,double y)
{

    xpoint[pos]=x;
    ypoint[pos]=y;
}

Dataset::~Dataset()
{
	auto pos1 = xpoint.begin();
	auto last1 = xpoint.end();
	xpoint.erase(pos1,last1);
	auto pos2=ypoint.begin();
	auto end2=ypoint.end();
	ypoint.erase(pos2,end2);
}
