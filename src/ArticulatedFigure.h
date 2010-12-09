#ifndef ARTICULATEDFIGURE_H
#define ARTICULATEDFIGURE_H

#include <cmlwrapper.h>
#include <vector>
#include <assert.h>
#include <iostream>
#include "Logging.h"

/** \brief General types of joints
 *
 * \todo add prismatic joints
 * \todo add proper fixed joint handling
 */
enum JointType {
	JointTypeUndefined = 0,
	JointTypeFixed,
	JointTypeRevolute
};

/** \brief Describes all properties of a single body 
 *
 * A Body contains information about mass, the location of its center of mass,
 * and the ineria tensor in the center of mass. This class is used to use the
 * given information and transform it such that it can directly be used by the
 * spatial algebra.
 */
struct Body {
	Body() :
		mSpatialInertia (
				0., 0., 0., 0., 0., 0.,	
				0., 0., 0., 0., 0., 0.,	
				0., 0., 0., 0., 0., 0.,	
				0., 0., 0., 0., 0., 0.,	
				0., 0., 0., 0., 0., 0.,	
				0., 0., 0., 0., 0., 0.
				),
		mCenterOfMass (0., 0., 0.),
		mMass (1.) { };
	Body(const Body &body) :
		mSpatialInertia (body.mSpatialInertia),
		mCenterOfMass (body.mCenterOfMass),
		mMass (body.mMass) {};
	Body& operator= (const Body &body) {
		if (this != &body) {
			mSpatialInertia = body.mSpatialInertia;
			mCenterOfMass = body.mCenterOfMass;
			mMass = body.mMass;
		}
	}
	/** \brief Constructs a body out of the given parameters
	 *
	 * This constructor eases the construction of a new body as all the required
	 * parameters can simply be specified as parameters to the constructor.
	 * These are then used to generate the spatial inertia matrix.
	 *
	 * \param mass the mass of the body
	 * \param com  the position of the center of mass in the bodies coordinates
	 * \param gyration_radii the radii of gyration at the center of mass of the body
	 */
	Body(const double &mass,
			const Vector3d &com,
			const Vector3d &gyration_radii) :
		mMass (mass),
		mCenterOfMass(com) {
			Matrix3d com_cross (
					0., -com[2],  com[1],
					com[2],      0., -com[0],
					-com[1],  com[0],      0.
					);
			Matrix3d parallel_axis;
			parallel_axis = mass * com_cross * cml::transpose(com_cross);

			LOG << "parrallel axis = " << parallel_axis << std::endl;

			Vector3d gr (gyration_radii);
			Matrix3d pa (parallel_axis);
			Matrix3d mcc = mass * com_cross;
			Matrix3d mccT = transpose(mcc);

			mSpatialInertia.set (
					gr[0] + pa(0, 0), pa(0, 1), pa(0, 2), mcc(0, 0), mcc(0, 1), mcc(0, 2),
					pa(1, 0), gr[1] + pa(1, 1), pa(1, 2), mcc(1, 0), mcc(1, 1), mcc(1, 2),
					pa(2, 0), pa(2, 1), gr[2] + pa(2, 2), mcc(2, 0), mcc(2, 1), mcc(2, 2),
					mccT(0, 0), mccT(0, 1), mccT(0, 2), mass, 0., 0.,
					mccT(1, 0), mccT(1, 1), mccT(1, 2), 0., mass, 0.,
					mccT(2, 0), mccT(2, 1), mccT(2, 2), 0., 0., mass
					);
		}

	~Body() {};

	/// \brief The spatial inertia that contains both mass and inertia information
	SpatialAlgebra::SpatialMatrix mSpatialInertia;
	/// \brief The position of the center of mass in body coordinates
	Vector3d mCenterOfMass;
	/// \brief The mass of the body
	double mMass;
};

/** \brief Describes a joint relative to the predecessor body 
 *
 * This class contains all information required for one single joint. This
 * contains the joint type and the axis of the joint.
 */
struct Joint {
	Joint() :
		mJointAxis (
				0., 0., 0.,
				0., 0., 0.
				),
		mJointType (JointTypeUndefined) {};
	Joint (const Joint &joint) :
		mJointAxis (joint.mJointAxis),
		mJointType (joint.mJointType) {};
	Joint& operator= (const Joint &joint) {
		if (this != &joint) {
			mJointAxis = joint.mJointAxis;
			mJointType = joint.mJointType;
		}
		return *this;
	}

	/** \brief Constructs a joint from the given cartesian parameters
	 *
	 * This constructor creates all the required spatial values for the given
	 * cartesian parameters.
	 *
	 * \param joint_type whether the joint is revolute or prismatic
	 * \param joint_axis the axis of rotation or translation
	 */
	Joint (
			const JointType joint_type,
			const Vector3d &joint_axis
			) {
		// Some assertions, as we concentrate on simple cases

		// Only rotation around the Z-axis
		assert ( joint_type == JointTypeRevolute || joint_type == JointTypeFixed
				);
		mJointType = joint_type;

		if (joint_type == JointTypeRevolute) {
			assert ( joint_axis == Vector3d (1., 0., 0.)
					|| joint_axis == Vector3d (0., 1., 0.)
					|| joint_axis == Vector3d (0., 0., 1.));

			mJointAxis.set (
					joint_axis[0],
					joint_axis[1], 
					joint_axis[2], 
					0., 0., 0.
					);

		} else if (joint_type == JointTypeFixed) {
			mJointAxis.set (
					joint_axis[0],
					joint_axis[1], 
					joint_axis[2], 
					0., 0., 0.
					);
			mJointAxis.set (0., 0., 0., 0., 0., 0.);
		}
	}

	/// \brief The spatial axis of the joint
	SpatialAlgebra::SpatialVector mJointAxis;
	/// \brief Type of joint (rotational or prismatic)
	JointType mJointType;
};

/** \brief Class that contains information for an body-environment contact
 */
struct Contact {
	Contact() :
		mBodyId (0),
		mPoint (0., 0., 0.)
	{}
	Contact (const Contact &contact):
		mBodyId(contact.mBodyId),
		mPoint(contact.mPoint)
	{}
	Contact& operator= (const Contact &contact) {
		if (this != &contact) {
			mBodyId = contact.mBodyId;
			mPoint = contact.mPoint;
		}
		return *this;
	}
	~Contact() {};

	Contact (const unsigned int &body_id, const Vector3d &point) :
		mBodyId(body_id),
		mPoint(point)
	{ }

	unsigned int mBodyId;
	Vector3d mPoint;
};

/** \brief Contains all information of the model
 *
 * This class contains all information required to perform the forward
 * dynamics calculation. The variables in this class are also used for storage
 * of temporary values. It is designed for use of the Composite Rigid Body
 * Algorithm and follows the numbering as described in Featherstones book.
 *
 * An important note is that body 0 is the root body and the moving bodies
 * start at index 1. Additionally the vectors for the states q, qdot, etc.
 * have #bodies + 1 entries where always the first entry (e.g. q[0]) contains
 * the value for the root body. Thus the numbering might be confusing as q[1]
 * holds the position variable of the first degree of freedom. This numbering
 * scheme is very benefial in terms of readability of the code as the
 * resulting code is very similar to the pseudo-code in the RBDA book.
 */
struct Model {
	// Structural information

	/// \brief The id of the parents body
	std::vector<unsigned int> lambda;

	/// \brief true if the body has a floating base
	bool floating_base;
	/// \brief the cartestian translation of the base
	Vector3d base_translation;
	/// \brief the rotation of the base in ZYX-Euler angles
	Vector3d base_rotation;

	/// \brief the cartesian vector of the gravity
	Vector3d gravity;

	// State information

	/** \brief The joint position
	 * 
	 * Warning: to have an easier numbering in the algorithm the state vector
	 * has NDOF + 1 elements. However element with index 0 is not used!
	 * 
	 * q[0] - unused
	 * q[1] - joint 1
	 * q[2] - joint 2
	 * ...
	 * q[NDOF] - joint NDOF
	 *
	 */
	std::vector<double> q;
	/// \brief The joint velocity
	std::vector<double> qdot;
	/// \brief The joint acceleration
	std::vector<double> qddot;
	/// \brief The force / torque applied at joint i
	std::vector<double> tau;
	/// \brief The spatial velocity of body i
	std::vector<SpatialAlgebra::SpatialVector> v;
	/// \brief The spatial acceleration of body i
	std::vector<SpatialAlgebra::SpatialVector> a;

	////////////////////////////////////
	// Joints\t

	/// \brief All joints
	std::vector<Joint> mJoints;
	/// \brief The joint axis for joint i
	std::vector<SpatialAlgebra::SpatialVector> S;
	/// \brief Transformations from the parent body to the frame of the joint
	std::vector<SpatialAlgebra::SpatialMatrix> X_T;

	////////////////////////////////////
	// Dynamics variables

	/// \brief The velocity dependent spatial acceleration
	std::vector<SpatialAlgebra::SpatialVector> c;
	/// \brief The spatial inertia of body i
	std::vector<SpatialAlgebra::SpatialMatrix> IA;
	/// \brief The spatial bias force
	std::vector<SpatialAlgebra::SpatialVector> pA;
	/// \brief Temporary variable U_i (RBDA p. 130)
	std::vector<SpatialAlgebra::SpatialVector> U;
	/// \brief Temporary variable D_i (RBDA p. 130)
	std::vector<double> d;
	/// \brief Temporary variable u (RBDA p. 130)
	std::vector<double> u;

	////////////////////////////////////
	// Bodies

	/// \brief Transformation from the parent body to the current body
	std::vector<SpatialAlgebra::SpatialMatrix> X_lambda;
	/// \brief Transformation from the base to bodies reference frame
	std::vector<SpatialAlgebra::SpatialMatrix> X_base;

	/** \brief All bodies 0 ... N_B, including the base
	 * mBodies[0] - base body
	 * mBodies[1] - 1st movable body
	 * ...
	 * mBodies[N_B] - N_Bth movable body
	 */
	std::vector<Body> mBodies;
	std::vector<Matrix3d> mBodyOrientation;

	/** \brief All contacts */
	std::vector<Contact> mContacts;

	/// \brief Initializes the helper values for the dynamics algorithm
	void Init ();
	/** \brief Connects a given body to the model
	 *
	 * \param parent_id   id of the parend body
	 * \param joint_frame the transformation from the parent frame to the origin
	 *                    of the joint frame (represents X_T in RBDA)
	 * \param joint       specification for the joint that describes the connection
	 * \param body        specification of the body itself
	 *
	 * \returns id of the added body
	 */
	unsigned int AddBody (
			const unsigned int parent_id,
			const SpatialAlgebra::SpatialMatrix &joint_frame,
			const Joint &joint,
			const Body &body
			);
	void SetFloatingBody (
			const Body &body
			);
	unsigned int AddContact (
			const unsigned int body_id,
			const Vector3d &contact_point
			);
};

/** \brief Computes the joint variables 
 *
 * \param model    the rigid body model
 * \param joint_id the id of the joint we are interested in (output)
 * \param XJ       the joint transformation (output)
 * \param S        motion subspace of the joint (output)
 * \param v_J      joint velocity (output)
 * \param c_J      joint acceleration for rhenomic joints (output)
 * \param q        joint state variable
 * \param qdot     joint velocity variable
 */
void jcalc (
		const Model &model,
		const unsigned int &joint_id,
		SpatialAlgebra::SpatialMatrix &XJ,
		SpatialAlgebra::SpatialVector &S,
		SpatialAlgebra::SpatialVector &v_J,
		SpatialAlgebra::SpatialVector &c_J,
		const double &q,
		const double &qdot
		);

/** \brief Computes forward dynamics for models with a fixed base
 *
 * \param model rigid body model
 * \param Q     state vector of the internal joints
 * \param QDot  velocity vector of the internal joints
 * \param Tau   actuations of the internal joints
 * \param QDDot accelerations of the internals joints (output)
 */
void ForwardDynamics (
		Model &model,
		const std::vector<double> &Q,
		const std::vector<double> &QDot,
		const std::vector<double> &Tau,
		std::vector<double> &QDDot
		);

/** \brief Computes forward dynamics for models with a floating base
 *
 * \param model rigid body model
 * \param Q     state vector of the internal joints
 * \param QDot  velocity vector of the internal joints
 * \param Tau   actuations of the internal joints
 * \param X_B   transformation into base coordinates
 * \param v_B   velocity of the base (in base coordinates)
 * \param f_B   forces acting on the base (in base coordinates)
 * \param a_B   accelerations of the base (output, in base coordinates)
 * \param QDDot accelerations of the internals joints (output)
 */
void ForwardDynamicsFloatingBase (
		Model &model,
		const std::vector<double> &Q,
		const std::vector<double> &QDot,
		const std::vector<double> &Tau,
		const SpatialAlgebra::SpatialMatrix &X_B,
		const SpatialAlgebra::SpatialVector &v_B,
		const SpatialAlgebra::SpatialVector &f_B,
		SpatialAlgebra::SpatialVector &a_B,
		std::vector<double> &QDDot
		);

/** \brief Computes the velocity of a point on a body 
 *
 * \param model   rigid body model
 * \param Q       state vector of the internal joints
 * \param QDot    velocity vector of the internal joints
 * \param body_id the id of the body
 * \param point_position the position of the point in body-local data
 * \param point_velocity cartesian velocity of the point in global frame (output)
 */
void CalcPointVelocity (
		Model &model,
		const std::vector<double> &Q,
		const std::vector<double> &QDot,
		unsigned int body_id,
		const Vector3d &point_position,
		Vector3d &point_velocity
		);

/** \brief Computes the acceleration of a point on a body 
 *
 * \param model   rigid body model
 * \param Q       state vector of the internal joints
 * \param QDot    velocity vector of the internal joints
 * \param QDDot    velocity vector of the internal joints
 * \param body_id the id of the body
 * \param point_position the position of the point in body-local data
 * \param point_acceleration cartesian velocity of the point in global frame (output)
 */
void CalcPointAcceleration (
		Model &model,
		const std::vector<double> &Q,
		const std::vector<double> &QDot,
		const std::vector<double> &QDDot,
		unsigned int body_id,
		const Vector3d &point_position,
		Vector3d &point_acceleration
		);

#endif /* ARTICULATEDFIGURE_H */
