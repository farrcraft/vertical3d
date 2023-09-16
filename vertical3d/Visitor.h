#ifndef INCLUDED_V3D_VISITOR
#define INCLUDED_V3D_VISITOR

namespace v3D
{

    class Visitor
    {
        public:
            Visitor();
            virtual ~Visitor();

            //virtual void visit(Particle * particle) = 0;

        private:
    };

}; // end namespace v3D


#endif // INCLUDED_V3D_VISITOR
