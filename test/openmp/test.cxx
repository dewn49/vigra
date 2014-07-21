#include <functional>
#include "vigra/unittest.hxx"
#include "vigra/stdimage.hxx"
#include "vigra/openmp_vigra.h"
#include "vigra/multi_array.hxx"
#include "vigra/error.hxx"

using namespace std;
using namespace vigra;

//#define PARA_CHECK;
//#ifdef OPENMP

class ParallelTestFunctor
{
	int ID;
	ParallelTestFunctor(ParallelTestFunctor const&) :
			ID(omp_get_thread_num()) {
	}

	template<class T>
	T operator()(T const& arg1, T const& arg2) const {
		return arg1 + arg2 + ID;
	}
};

class CountIterationFunctor
{
private:
	unsigned int iteration_num;

public:
	CountIterationFunctor()
	:iteration_num(0)
	{ }

	void operator()()
	{
#ifdef OPENMP
	#pragma omp critical //#pragma omp atomic update
#endif
		iteration_num++;
	}

	unsigned int getIterationNum()
	{
		return iteration_num;
	}
};

struct OpenMPWrapperTest {
	typedef vigra::DImage Image;

	OpenMPWrapperTest() :
			img(10000, 10000), mask(Shape2(10000, 10000)) {
		Image::Accessor acc = img.accessor();
		Image::ScanOrderIterator i = img.begin();

		for (int j = 1; i < img.end(); ++i, ++j) {
			acc.set(j, i);
		}

		mask.init(1);
		mask.begin()[0] = 0;
		mask.begin()[10000*10000-1] = 0;
	}

	void copyImageTest() {
		Image img1(10000, 10000);

		CountIterationFunctor functor;

		vigra::omp::copyImage(srcImageRange(img), destImage(img1), functor);

		Image::ScanOrderIterator i = img.begin();
		Image::ScanOrderIterator i1 = img1.begin();
		Image::Accessor acc = img.accessor();

#ifndef PARA_CHECK
		for ( ; i != img.end(); ++i, ++i1)
		{
			should(acc(i) == acc(i1));
		}
#else
		int onetheard = (img1.width() * img1.height()) / 3;
#pragma omp parallel sections
		{
#pragma omp section
			{
				i1 = img1.begin();
				i = img.begin();
				std::cout << "First id = " << omp_get_thread_num() << std::endl;
				for ( ; i != img.begin() + onetheard; ++i, ++i1)
				{
					should(acc(i) == acc(i1));
				}
			}
#pragma omp section
			{
				i1 = img1.begin() + onetheard;
				i = img.begin() + onetheard;
				std::cout << "Second id = " << omp_get_thread_num() << std::endl;
				for ( ; i != img.begin()+ 2*onetheard; ++i, ++i1)
				{
					should(acc(i) == acc(i1));
				}
			}
#pragma omp section
			{
				i1 = img1.begin() + 2 * onetheard;
				i = img.begin() + 2 * onetheard;
				std::cout << "Third id = " << omp_get_thread_num() << std::endl;
				for ( ; i != img.end(); ++i, ++i1)
				{
					should(acc(i) == acc(i1));
				}
			}
		} //omp sections
#endif //PARA_CHECK

#ifdef OPENMP
		should(functor.getIterationNum() == img.height());
#endif

	}

	void combineTwoImagesTest()
	{
		Image img1(10000, 10000);

		CountIterationFunctor iter_count;

		std::plus<Image::value_type> add;

		vigra::omp::combineTwoImages(srcImageRange(img), srcImage(img), destImage(img1), add, iter_count);

		using namespace functor; //TODO: For what?

		Image::ScanOrderIterator i = img.begin();
		Image::ScanOrderIterator i1 = img1.begin();
		Image::Accessor acc = img.accessor();

		for(; i != img.end(); ++i, ++i1)
		{
			should(2.0*acc(i) == acc(i1));
		}

#ifdef OPENMP
		should( iter_count.getIterationNum() == img.height());
#endif

	}

	void combineTwoImagesIfTest()
	{
		Image img1(10000, 10000);
		img1 = 27.0;

		CountIterationFunctor iter_count;

		std::plus<Image::value_type> add;

		vigra::omp::combineTwoImagesIf(srcImageRange(img), srcImage(img), maskImage(mask), destImage(img1), add, iter_count);

		using namespace functor; //TODO: For what?

		Image::ScanOrderIterator i = img.begin();
		Image::ScanOrderIterator i1 = img1.begin();
		Image::ScanOrderIterator i1end = img1.end();
		Image::Accessor acc = img.accessor();

		should(acc(i1) == 27.0);

			i++;
			i1++;
			i1end--;

		should(acc(i1end) == 27.0);

		for(; i1 != i1end; ++i, ++i1)
		{
			should(2.0*acc(i) == acc(i1));
		}

#ifdef OPENMP
		should( iter_count.getIterationNum() == img.height());
#endif

	}

	void combineThreeImagesTest()
	{
		Image img1(10000, 10000);
		Image img2(10000, 10000);

		CountIterationFunctor iter_count;
		CountIterationFunctor iter_count1;

		std::plus<Image::value_type> add;

		vigra::omp::combineTwoImages(srcImageRange(img), srcImage(img), destImage(img1), add, iter_count);

		Image::ScanOrderIterator i = img.begin();
		Image::ScanOrderIterator i1 = img1.begin();
		Image::Accessor acc = img.accessor();

		for(; i != img.end(); ++i, ++i1)
		{
			should(2.0*acc(i) == acc(i1));
		}

#ifdef OPENMP
		should( iter_count.getIterationNum() == img.height());
#endif

		using namespace functor; //To get Arg1(), Arg2(), Arg3() ??
		vigra::omp::combineThreeImages(srcImageRange(img), srcImage(img), srcImage(img1), destImage(img2), Arg1() + Arg2() + Arg3(), iter_count1);

		i = img.begin();
		Image::ScanOrderIterator i2 = img2.begin();

		for(; i != img.end(); ++i, ++i2)
		{
			should(4.0*acc(i) == acc(i2));
		}

#ifdef OPENMP
		should( iter_count1.getIterationNum() == img.height());
#endif

	}

	void transformImageTest()
	{
		Image img1(10000, 10000);

		CountIterationFunctor iter_count;

		vigra::omp::transformImage(srcImageRange(img), destImage(img1),
				linearIntensityTransform<Image::value_type>(3.3), iter_count);

		Image::ScanOrderIterator i = img.begin();
		Image::ScanOrderIterator i1 = img1.begin();
		Image::Accessor acc = img.accessor();

		for(; i != img.end(); ++i, ++i1)
		{
			should(3.3*acc(i) == acc(i1));
		}

#ifdef OPENMP
		should( iter_count.getIterationNum() == img.height());
#endif

	}

	void transformImageIfTest()
	{
		Image img1(10000, 10000);
		img1 = 27.0;

		CountIterationFunctor iter_count;

		vigra::omp::transformImageIf(srcImageRange(img), maskImage(mask), destImage(img1),
				linearIntensityTransform<Image::value_type>(3.3), iter_count);

		Image::ScanOrderIterator i = img.begin();
		Image::ScanOrderIterator i1 = img1.begin();
		Image::ScanOrderIterator i1end = img1.end();
		Image::Accessor acc = img.accessor();

		should(acc(i1) == 27.0);

		i++;
		i1++;
		i1end--;

		should(acc(i1end) == 27.0);

		for(; i1 != i1end; ++i, ++i1)
		{
			should(3.3*acc(i) == acc(i1));
		}

#ifdef OPENMP
		should( iter_count.getIterationNum() == img.height());
#endif

	}

	Image img;
	vigra::MultiArray<2, unsigned char> mask;
};

struct OpenMPWrapperTestSuite: public vigra::test_suite {
	OpenMPWrapperTestSuite() :
			vigra::test_suite("OpenMPWrapperTestSuite") {

		add( testCase( &OpenMPWrapperTest::copyImageTest));
		add( testCase( &OpenMPWrapperTest::combineTwoImagesTest));
		add( testCase( &OpenMPWrapperTest::transformImageTest));
		add( testCase( &OpenMPWrapperTest::combineThreeImagesTest));
		add( testCase( &OpenMPWrapperTest::transformImageIfTest));
		add( testCase( &OpenMPWrapperTest::combineTwoImagesIfTest));
	}
};

int main(int argc, char ** argv) {

	OpenMPWrapperTestSuite test;

	int failed = test.run(vigra::testsToBeExecuted(argc, argv));

	std::cout << test.report() << std::endl;

	return (failed != 0);
}

//#endif //OpenMP
